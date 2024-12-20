package api

import (
	"Havoc/pkg"
	"Havoc/pkg/cert"
	"Havoc/pkg/colors"
	"Havoc/pkg/logger"
	"Havoc/pkg/utils"
	"crypto/rand"
	"crypto/sha256"
	"crypto/x509"
	"encoding/hex"
	"encoding/json"
	"encoding/pem"
	"errors"
	"fmt"
	"github.com/gin-gonic/gin"
	"github.com/gorilla/websocket"
	"io"
	"net/http"
	"os"
	"sync"
)

const (
	ApiMaxRequestRead = 50000000 // 50mb
	ApiTokenLength    = 32
	ApiTokenHeader    = "x-havoc-token"
)

const (
	UserStatusOffline = 0
	UserStatusOnline  = 1
)

type ServerApi struct {
	engine *gin.Engine

	// ssl structure containing the path to
	// the generated ssl cert and key file
	ssl struct {
		cert string
		key  string
	}

	// havoc core interface
	// to interact with some functions to add, query,
	// remove objects and data to from the teamserver
	pkg.IHavocCore

	// wait queue for websockets to send
	// the access token to register the
	// event endpoint
	tokens sync.Map
}

func init() {
	gin.SetMode(gin.ReleaseMode)
}

func NewServerApi(havoc pkg.IHavocCore) (*ServerApi, error) {
	var api = &ServerApi{
		IHavocCore: havoc,
		engine:     gin.New(),
	}

	//
	// login & logout endpoint
	api.engine.POST("/api/login", api.login)
	api.engine.POST("/api/logout", api.logout)

	//
	// listeners endpoints
	api.engine.POST("/api/listener/list", api.listenerList)
	api.engine.POST("/api/listener/start", api.listenerStart)
	api.engine.POST("/api/listener/stop", api.listenerStop)
	api.engine.POST("/api/listener/restart", api.listenerRestart)
	api.engine.POST("/api/listener/remove", api.listenerRemove)
	api.engine.POST("/api/listener/edit", api.listenerEdit)
	api.engine.POST("/api/listener/event", api.listenerEvent)
	api.engine.POST("/api/listener/config", api.listenerConfig)

	//
	// agent endpoints
	api.engine.POST("/api/agent/list", api.agentList)
	api.engine.POST("/api/agent/build", api.agentBuild)
	api.engine.POST("/api/agent/execute", api.agentExecute)
	api.engine.POST("/api/agent/note", api.agentNote)
	api.engine.POST("/api/agent/console", api.agentConsole)
	api.engine.POST("/api/agent/remove", api.agentRemove)

	//
	// plugin api endpoints
	api.engine.POST("/api/plugin/list", api.pluginList)
	api.engine.POST("/api/plugin/resource", api.pluginResource)

	//
	// websocket event endpoint
	api.engine.GET("/api/event", api.event)

	return api, nil
}

// Start the server api
// generates an HTTP certificate and then starts the api server
func (api *ServerApi) Start(host, port string) {
	var (
		err  error
		file []byte
		ssl  *x509.Certificate
		hash [32]byte
	)

	// read the ssl sha256 hash fingerprint to present it to
	// the server console so the operator can validate the
	// connection being secure between the server and client

	// read the specified ssl certificate into a buffer
	if file, err = os.ReadFile(api.ssl.cert); err != nil {
		logger.Error("ssl cert read error: %v", err)
		return
	}

	// decode the PEM block containing the certificate
	block, _ := pem.Decode(file)
	if block == nil || block.Type != "CERTIFICATE" {
		logger.Error("failed to parse certificate: couldn't decode PEM block containing the certificate")
		return
	}

	// parse the ssl certificate
	if ssl, err = x509.ParseCertificate(block.Bytes); err != nil {
		logger.Error("failed to parse certificate: %v", err)
		return
	}

	// compute the SHA-256 hash of the certificate's raw bytes
	// and then log it into the console
	hash = sha256.Sum256(ssl.Raw)
	logger.Info("ssl fingerprint: %v", colors.BoldGreen(hex.EncodeToString(hash[:])))

	// now start the api server so clients can connect
	// and interact with the agents, listeners, etc.
	logger.Info("serving server on %v", colors.BlueUnderline("https://"+host+":"+port))
	if err = api.engine.RunTLS(host+":"+port, api.ssl.cert, api.ssl.key); err != nil {
		logger.Error("failed to start webserver: " + err.Error())
		return
	}
}

func (api *ServerApi) GenerateSSL(host, certsPath string) (string, string, error) {
	var (
		certPath = certsPath + "/server.cert"
		keyPath  = certsPath + "/server.key"
		err      error
		Cert     []byte
		Key      []byte
	)

	// generate cert and key based on host
	Cert, Key, err = cert.HTTPSGenerateRSACertificate(host)
	if err != nil {
		logger.Error("failed to generate server certificates: " + err.Error())
		os.Exit(0)
	}

	if _, err = os.Stat(certPath); os.IsNotExist(err) {
		// write cert file to disk
		err = os.WriteFile(certPath, Cert, 0644)
		if err != nil {
			return "", "", errors.New("couldn't save server cert file: " + err.Error())
		}
	}

	if _, err = os.Stat(keyPath); os.IsNotExist(err) {
		// write the cert key path to disk
		err = os.WriteFile(keyPath, Key, 0644)
		if err != nil {
			return "", "", errors.New("couldn't save server cert file: " + err.Error())
		}
	}

	return certPath, keyPath, nil
}

func (api *ServerApi) SetSSL(certPath, keyPath string) error {

	if _, err := os.Stat(certPath); os.IsNotExist(err) {
		return err
	}

	if _, err := os.Stat(keyPath); os.IsNotExist(err) {
		return err
	}

	api.ssl.cert = certPath
	api.ssl.key = keyPath

	return nil
}

// login endpoint for the client to log in
// and retrieve an access token
func (api *ServerApi) login(ctx *gin.Context) {
	var (
		body  []byte
		token string
		err   error
		user  string
		pass  string
		login map[string]any
	)

	// read from request the login data
	if body, err = io.ReadAll(io.LimitReader(ctx.Request.Body, ApiMaxRequestRead)); err != nil {
		logger.DebugError("Failed to read from server api login request: " + err.Error())
		goto ERROR
	}

	logger.Debug("got request on /api/login:" + fmt.Sprintf("%s", string(body)))

	// unmarshal the bytes into a map
	if err = json.Unmarshal(body, &login); err != nil {
		logger.DebugError("Failed to unmarshal bytes to map: " + err.Error())
		return
	}

	// generate a token
	// if failed then abort with status code
	if token = api.generateToken(); len(token) == 0 {
		ctx.AbortWithStatus(http.StatusInternalServerError)
		return
	}

	// get username from login request
	if user, err = utils.MapKey[string](login, "username"); err != nil {
		logger.DebugError("failed to retrieve username: " + err.Error())
		goto ERROR
	}

	// check if the specified user
	// is already active/online
	if api.UserStatus(user) == UserStatusOnline {
		ctx.JSON(http.StatusUnauthorized, gin.H{
			"error": "user is already online",
		})
		return
	}

	// get password from login request
	if pass, err = utils.MapKey[string](login, "password"); err != nil {
		logger.DebugError("failed to retrieve username: " + err.Error())
		goto ERROR
	}

	// check if we can authenticate the user
	if !api.UserAuthenticate(user, pass) {
		ctx.AbortWithStatus(http.StatusUnauthorized)
		return
	}

	// send json token response
	ctx.JSON(http.StatusOK, gin.H{
		"token": token,
	})

	// store the token inside the wait queue
	api.tokens.Store(token, login)

	return

ERROR:
	ctx.AbortWithStatus(http.StatusInternalServerError)
}

func (api *ServerApi) logout(ctx *gin.Context) {

}

// handleEventClient
// handles incoming event clients
//
// it is going to read the given token from the client and checks
// if it in the wait queue. After log in the token is going to be
// stored inside the wait queue til the client connects to the websocket
// endpoint and send the token it previously got from the /api/login
// endpoint. After all of this the teamserver is going to register
// the client as officially logged in.
//
//	socket *websocket.Conn
func (api *ServerApi) handleEventClient(socket *websocket.Conn) {
	var (
		body  []byte
		err   error
		data  map[string]any
		login any
		token string
		ok    bool
	)

	if socket == nil {
		return
	}

	// try to read the token from socket
	if _, body, err = socket.ReadMessage(); err != nil {
		logger.DebugError("failed reading message from socket: " + err.Error())
		goto ERROR
	}

	if len(body) < ApiTokenLength {
		logger.DebugError("invalid socket request. closing connection")
		goto ERROR
	}

	// unmarshal the body to a map
	if err = json.Unmarshal(body, &data); err != nil {
		logger.DebugError("failed unmarshal message from socket: " + err.Error())
		goto ERROR
	}

	// get token from request
	if token, err = utils.MapKey[string](data, "token"); err != nil {
		logger.DebugError("failed retrieving token: " + err.Error())
		goto ERROR
	}

	// check if is inside the tokens map
	if login, ok = api.tokens.Load(token); !ok {
		logger.DebugError("failed retrieve token from map: token entry not found")
		goto ERROR
	}

	// by now the client was successfully authenticated
	// we're going to register the user to the teamserver
	api.UserLogin(token, login, socket)

	// this loop is just there to check the state of the connection
	// and if the client somehow disconnected from the connection.
	// if the user disconnected then handle it by close the resources
	// related to that user session
	for {
		// check if any error arrived from reading any message or
		// checking the state of the connection
		if _, _, err = socket.ReadMessage(); err == nil {
			continue
		}

		logger.Debug("user connection closed [token: %v] -> %v", token, err)

		// log out the connected user client and send
		// a broadcast event to every connected client
		if err = api.UserLogoutByToken(token); err != nil {
			logger.DebugError("failed to logout user by token: %v", err)
		}

		break
	}

ERROR:
	socket.Close()
}

// sanityCheck
// checks if the request is coming form a valid client
func (api *ServerApi) sanityCheck(ctx *gin.Context) bool {
	var (
		token string
		ok    bool
	)

	// check if the token header exists
	if token = ctx.GetHeader(ApiTokenHeader); len(token) == 0 || len(token) < ApiTokenLength {
		return false
	}

	// check if is inside the tokens map
	if _, ok = api.tokens.Load(token); !ok {
		logger.DebugError("failed retrieve token from map: token entry not found")
		return false
	}

	return true
}

func (api *ServerApi) tokenUser(token string) (string, bool) {
	var (
		user  string
		val   any
		login map[string]any
		ok    bool
		err   error
	)

	// check if is inside the tokens map
	if val, ok = api.tokens.Load(token); !ok {
		logger.DebugError("failed retrieve token from map: token entry not found")
		return user, false
	}

	login = val.(map[string]any)

	// get name from agent request
	if user, err = utils.MapKey[string](login, "username"); err != nil {
		logger.DebugError("failed retrieve login username: %v", err)
		return "", false
	}

	return user, true
}

// generateToken
// generate a token for the client to use for authenticate
// and to use to interact with the api with
func (api *ServerApi) generateToken() string {
	b := make([]byte, ApiTokenLength/2)
	if _, err := rand.Read(b); err != nil {
		return ""
	}
	return hex.EncodeToString(b)
}
