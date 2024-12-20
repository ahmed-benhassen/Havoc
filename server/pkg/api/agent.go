package api

import (
	"Havoc/pkg/logger"
	"Havoc/pkg/utils"
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
		body   []byte
		config map[string]any
		agent  map[string]any
		cfg    map[string]any
		name   string
		err    error
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

	name, err = utils.MapKey[string](agent, "name")
	if err != nil {
		goto ERROR
	}

	config, err = utils.MapKey[map[string]any](agent, "config")
	if err != nil {
		goto ERROR
	}

	// interact with the plugin to generate a payload
	name, body, cfg, err = api.AgentGenerate(name, config)
	if err != nil {
		logger.DebugError("Failed to generate agent payload: %v", err)
		goto ERROR
	}

	// return base64 encoded payload
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

// agentExecute
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

	uuid, err = utils.MapKey[string](response, "uuid")
	if err != nil {
		goto ERROR
	}

	data, err = utils.MapKey[map[string]any](response, "data")
	if err != nil {
		goto ERROR
	}

	wait, err = utils.MapKey[bool](response, "wait")
	if err != nil {
		goto ERROR
	}

	// interact with the plugin to generate a payload
	response, err = api.AgentExecute(uuid, data, wait)
	if err != nil {
		logger.DebugError("Failed to execute agent command: %v", err)
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

	uuid, err = utils.MapKey[string](response, "uuid")
	if err != nil {
		goto ERROR
	}

	note, err = utils.MapKey[string](response, "note")
	if err != nil {
		goto ERROR
	}

	err = api.AgentSetNote(uuid, note)
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

func (api *ServerApi) agentList(ctx *gin.Context) {
	var (
		err    error
		array  []string
		list   []map[string]any
		agent  map[string]any
		plugin string
		note   string
		status string
	)

	if !api.sanityCheck(ctx) {
		ctx.AbortWithStatus(http.StatusUnauthorized)
		return
	}

	logger.Debug("/api/agent/list")

	array = api.AgentList()
	for _, uuid := range array {
		if plugin, err = api.AgentType(uuid); err != nil {
			logger.DebugError("failed to get agent type: %v", err)
			goto ERROR
		}

		if note, err = api.AgentNote(uuid); err != nil {
			logger.DebugError("failed to get agent note: %v", err)
			goto ERROR
		}

		if status, err = api.AgentStatus(uuid); err != nil {
			logger.DebugError("failed to get agent status: %v", err)
			goto ERROR
		}

		if agent, err = api.AgentData(uuid); err != nil {
			logger.DebugError("failed to get agent info: %v", err)
			goto ERROR
		}

		list = append(list, map[string]any{
			"uuid":   uuid,
			"type":   plugin,
			"meta":   agent,
			"note":   note,
			"status": status,
		})
	}

	// send back response
	ctx.JSON(http.StatusOK, list)
	return

ERROR:
	ctx.JSON(http.StatusInternalServerError, gin.H{
		"error": err.Error(),
	})
}

func (api *ServerApi) agentRemove(ctx *gin.Context) {
	var (
		err      error
		body     []byte
		uuid     string
		response map[string]any
	)

	if !api.sanityCheck(ctx) {
		ctx.AbortWithStatus(http.StatusUnauthorized)
		return
	}

	// read from request the login data
	if body, err = io.ReadAll(io.LimitReader(ctx.Request.Body, ApiMaxRequestRead)); err != nil {
		logger.DebugError("failed to read from server api login request: " + err.Error())
		goto ERROR
	}

	logger.Debug("/api/agent/remove -> %v", string(body))

	// unmarshal the bytes into a map
	if err = json.Unmarshal(body, &response); err != nil {
		logger.DebugError("failed to unmarshal bytes to map: " + err.Error())
		err = errors.New("invalid request")
		goto ERROR
	}

	if uuid, err = utils.MapKey[string](response, "uuid"); err != nil {
		goto ERROR
	}

	if err = api.AgentRemove(uuid); err != nil {
		logger.DebugError("failed to delete agent session: %v", err)
		goto ERROR
	}

	ctx.AbortWithStatus(http.StatusOK)
	return
ERROR:
	ctx.JSON(http.StatusInternalServerError, gin.H{
		"error": err.Error(),
	})
}

func (api *ServerApi) agentConsole(ctx *gin.Context) {
	var (
		err      error
		body     []byte
		uuid     string
		response map[string]any
		list     []map[string]any
	)

	if !api.sanityCheck(ctx) {
		ctx.AbortWithStatus(http.StatusUnauthorized)
		return
	}

	// read from request the login data
	if body, err = io.ReadAll(io.LimitReader(ctx.Request.Body, ApiMaxRequestRead)); err != nil {
		logger.DebugError("failed to read from server api login request: " + err.Error())
		goto ERROR
	}

	logger.Debug("/api/agent/console -> %v", string(body))

	// unmarshal the bytes into a map
	if err = json.Unmarshal(body, &response); err != nil {
		logger.DebugError("failed to unmarshal bytes to map: " + err.Error())
		err = errors.New("invalid request")
		goto ERROR
	}

	if uuid, err = utils.MapKey[string](response, "uuid"); err != nil {
		goto ERROR
	}

	if list, err = api.DatabaseAgentConsole(uuid); err != nil {
		logger.DebugError("failed to get agent console: %v", err)
		goto ERROR
	}

	ctx.JSON(http.StatusOK, list)
	return

ERROR:
	ctx.JSON(http.StatusInternalServerError, gin.H{
		"error": err.Error(),
	})
}
