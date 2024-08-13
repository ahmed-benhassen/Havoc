package api

import (
	"Havoc/pkg/logger"
	"encoding/base64"
	"encoding/json"
	"errors"
	"fmt"
	"github.com/gin-gonic/gin"
	"io"
	"net/http"
)

func (api *ServerApi) agentBuild(ctx *gin.Context) {
	var (
		body     []byte
		context  map[string]any
		config   map[string]any
		agent    map[string]any
		cfg      map[string]any
		name     string
		username string
		ok       bool
		err      error
	)

	if !api.sanityCheck(ctx) {
		ctx.AbortWithStatus(http.StatusUnauthorized)
		return
	}

	// read from request the login data
	if body, err = io.ReadAll(io.LimitReader(ctx.Request.Body, ApiMaxRequestRead)); err != nil {
		logger.DebugError("Failed to read from server api login request: " + err.Error())
		goto ERROR
	}

	logger.Debug("/api/agent/build -> %v", string(body))

	// unmarshal the bytes into a map
	if err = json.Unmarshal(body, &agent); err != nil {
		logger.DebugError("Failed to unmarshal bytes to map: " + err.Error())
		err = errors.New("invalid request")
		goto ERROR
	}

	//
	// get name from agent request
	//
	switch agent["name"].(type) {
	case string:
		name = agent["name"].(string)
	default:
		logger.DebugError("Failed retrieve agent name: invalid type")
		err = errors.New("invalid request")
		goto ERROR
	}

	//
	// get config from agent request
	//
	switch agent["config"].(type) {
	case map[string]any:
		config = agent["config"].(map[string]any)
	default:
		logger.DebugError("Failed retrieve agent config: invalid type")
		err = errors.New("invalid request")
		goto ERROR
	}

	//
	// get the username from the token
	//
	if username, ok = api.tokenUser(ctx.GetHeader(ApiTokenHeader)); !ok {
		logger.DebugError("Failed to authenticate token: couldn't get username from token")
		err = errors.New("failed to authenticate token")
		goto ERROR
	}

	context = map[string]any{
		"name": name,
		"user": username,
	}

	//
	// interact with the plugin to generate a payload
	//
	name, body, cfg, err = api.teamserver.AgentGenerate(context, config)
	if err != nil {
		logger.DebugError("Failed to generate agent payload: %v", err)
		goto ERROR
	}

	//
	// return base64 encoded payload
	//
	ctx.JSON(http.StatusOK, gin.H{
		"payload":  base64.StdEncoding.EncodeToString(body),
		"filename": name,
		"context":  cfg,
	})

	return

ERROR:
	ctx.JSON(http.StatusInternalServerError, gin.H{
		"error": err.Error(),
	})
}

func (api *ServerApi) agentExecute(ctx *gin.Context) {
	var (
		body     []byte
		err      error
		response map[string]any
		uuid     string
		data     map[string]any
		wait     bool
	)

	if !api.sanityCheck(ctx) {
		ctx.AbortWithStatus(http.StatusUnauthorized)
		return
	}

	// read from request the login data
	if body, err = io.ReadAll(io.LimitReader(ctx.Request.Body, ApiMaxRequestRead)); err != nil {
		logger.DebugError("Failed to read from server api login request: " + err.Error())
		goto ERROR
	}

	logger.Debug("/api/agent/execute -> %v", string(body))

	// unmarshal the bytes into a map
	if err = json.Unmarshal(body, &response); err != nil {
		logger.DebugError("Failed to unmarshal bytes to map: " + err.Error())
		err = errors.New("invalid request")
		return
	}

	// get uuid from client request
	switch response["uuid"].(type) {
	case string:
		uuid = response["uuid"].(string)
	default:
		logger.DebugError("Failed retrieve agent uuid: invalid type")
		err = errors.New("invalid request")
		goto ERROR
	}

	// get data from client request
	switch response["data"].(type) {
	case map[string]any:
		data = response["data"].(map[string]any)
	default:
		logger.DebugError("Failed retrieve agent data: invalid type")
		err = errors.New("invalid request")
		goto ERROR
	}

	// get wait status from client request
	switch response["wait"].(type) {
	case bool:
		wait = response["wait"].(bool)
	default:
		logger.DebugError("Failed retrieve agent wait status: invalid type")
		err = errors.New("invalid request")
		goto ERROR
	}

	// interact with the plugin to generate a payload
	response, err = api.teamserver.AgentExecute(uuid, data, wait)
	if err != nil {
		logger.DebugError("Failed to execute agent command: %v", err)
		err = fmt.Errorf("failed to execute command: %v", err)
		goto ERROR
	}

	// send back response
	ctx.JSON(http.StatusOK, response)
	return

ERROR:
	ctx.JSON(http.StatusInternalServerError, gin.H{
		"error": err.Error(),
	})
}

func (api *ServerApi) agentNote(ctx *gin.Context) {
	var (
		body     []byte
		err      error
		response map[string]any
		uuid     string
		note     string
	)

	if !api.sanityCheck(ctx) {
		ctx.AbortWithStatus(http.StatusUnauthorized)
		return
	}

	// read from request the login data
	if body, err = io.ReadAll(io.LimitReader(ctx.Request.Body, ApiMaxRequestRead)); err != nil {
		logger.DebugError("Failed to read from server api login request: " + err.Error())
		goto ERROR
	}

	logger.Debug("/api/agent/note -> %v", string(body))

	// unmarshal the bytes into a map
	if err = json.Unmarshal(body, &response); err != nil {
		logger.DebugError("failed to unmarshal bytes to map: " + err.Error())
		err = errors.New("invalid request")
		return
	}

	if val, ok := response["uuid"]; ok {
		// get uuid from client request
		switch val.(type) {
		case string:
			uuid = val.(string)
		default:
			logger.DebugError("failed retrieve agent uuid: invalid type")
			err = errors.New("invalid request")
			goto ERROR
		}
	} else {
		err = errors.New("invalid request")
		goto ERROR
	}

	if val, ok := response["note"]; ok {
		// get uuid from client request
		switch val.(type) {
		case string:
			note = response["note"].(string)
		default:
			logger.DebugError("failed retrieve agent note: invalid type")
			err = errors.New("invalid request")
			goto ERROR
		}
	} else {
		err = errors.New("invalid request")
		goto ERROR
	}

	err = api.teamserver.AgentNote(uuid, note)
	if err != nil {
		err = fmt.Errorf("failed to set agent note: %v", err)
		goto ERROR
	}

	// send back response
	ctx.JSON(http.StatusOK, response)
	return

ERROR:
	ctx.JSON(http.StatusInternalServerError, gin.H{
		"error": err.Error(),
	})
}
