package server

import (
	"Havoc/pkg/logger"
	"bytes"
	"encoding/gob"
	"encoding/json"
	"errors"
	"fmt"
)

func (t *Teamserver) AgentInitialize(uuid, plugin, status, note string, data map[string]any) error {
	var agent = &Agent{
		uuid:   uuid,
		plugin: plugin,
		note:   note,
		status: status,
	}

	// check if the given uuid already exists
	if _, ok := t.agents.Load(uuid); ok {
		return errors.New("agent with given uuid already exists")
	}

	// store the agent data with the given uuid
	t.agents.Store(uuid, agent)

	logger.Debug("agent: %v", agent)

	t.UserBroadcast(false, t.EventCreate(EventAgentInitialize, map[string]any{
		"uuid": uuid,
		"type": plugin,
		"meta": data,
	}))

	return nil
}

func (t *Teamserver) AgentExist(uuid string) bool {
	_, found := t.agents.Load(uuid)

	return found
}

func (t *Teamserver) AgentData(uuid string) (map[string]any, error) {
	var (
		agent *Agent
		val   any
		ok    bool
	)

	// check if the given uuid already exists
	if val, ok = t.agents.Load(uuid); ok {
		agent = val.(*Agent)
	} else {
		return nil, errors.New("agent with given uuid doesn't exists")
	}

	return t.plugins.AgentGet(agent.plugin, uuid)
}

func (t *Teamserver) AgentType(uuid string) (string, error) {
	var (
		agent any
		ok    bool
	)

	// check if the given uuid already exists
	if agent, ok = t.agents.Load(uuid); !ok {
		return "", errors.New("agent with given uuid doesn't exists")
	}

	return agent.(*Agent).plugin, nil
}

func (t *Teamserver) AgentRemove(uuid string) error {
	var (
		agent any
		ok    bool
		err   error
	)

	// check if the given uuid exists
	if agent, ok = t.agents.Load(uuid); !ok {
		return errors.New("agent with given uuid doesn't exists")
	}

	// interact with the plugin that manages the agent
	// to tell it that the agent is about to be removed
	err = t.plugins.AgentRemove(agent.(*Agent).plugin, uuid)
	if err != nil {
		logger.DebugError("AgentRemove error on %v: %v", uuid, err)
		return err
	}

	// delete it from the server map
	t.agents.Delete(uuid)

	t.UserBroadcast(false, t.EventCreate(EventAgentRemove, map[string]any{
		"uuid": uuid,
	}))

	return nil
}

func (t *Teamserver) AgentList() []string {
	var list []string

	t.agents.Range(func(k, v interface{}) bool {
		agent := v.(*Agent)
		list = append(list, agent.uuid)
		return true
	})

	return list
}

func (t *Teamserver) AgentNote(uuid string) (string, error) {
	var (
		agent any
		ok    bool
	)

	// check if the given uuid already exists
	if agent, ok = t.agents.Load(uuid); !ok {
		return "", errors.New("agent with given uuid doesn't exists")
	}

	return agent.(*Agent).note, nil
}

func (t *Teamserver) AgentSetNote(uuid, note string) error {
	var (
		agent *Agent
		value any
		ok    bool
	)

	// load stored agent by uuid from map
	if value, ok = t.agents.Load(uuid); ok {
		agent = value.(*Agent)
		agent.note = note

		// we honestly don't care much about it failed to update
		// continue and broadcast the status update
		_ = t.plugins.AgentUpdate(agent.plugin, uuid)

		t.UserBroadcast(false, t.EventCreate(EventAgentNote, map[string]any{
			"uuid": uuid,
			"note": note,
		}))

		return nil
	}

	return errors.New("agent by uuid not found")
}

// AgentSetStatus updates the agent object status
// and broadcasts the event to every connected client
func (t *Teamserver) AgentSetStatus(uuid string, status string) error {
	var agent *Agent

	// check if the given uuid already exists
	if val, ok := t.agents.Load(uuid); !ok {
		return errors.New("agent with given uuid doesn't exists")
	} else {
		agent = val.(*Agent)
	}

	agent.status = status

	// we honestly don't care much about it failed to update
	// continue and broadcast the status update
	_ = t.plugins.AgentUpdate(agent.plugin, uuid)

	t.UserBroadcast(false, t.EventCreate(EventAgentStatus, map[string]any{
		"uuid":   uuid,
		"status": status,
	}))

	return nil
}

func (t *Teamserver) AgentStatus(uuid string) (string, error) {
	var (
		agent any
		ok    bool
	)

	// check if the given uuid already exists
	if agent, ok = t.agents.Load(uuid); !ok {
		return "", errors.New("agent with given uuid doesn't exists")
	}

	return agent.(*Agent).status, nil
}

func (t *Teamserver) AgentRegisterType(name string, agent map[string]any) error {
	var (
		payload Handler
		data    []byte
		err     error
	)

	payload = Handler{
		Name: name,
		Data: agent,
	}

	// add the payload to the
	// available payloads lists
	t.payloads = append(t.payloads, payload)

	// convert the object to a json
	if data, err = json.Marshal(payload); err != nil {
		return err
	}

	// TODO: instead of using a broadcast + save use write an api endpoint for register

	// broadcast to all connected clients
	// and any future clients
	t.UserBroadcast(true, t.EventCreateJson(EventAgentRegister, data))

	return nil
}

func (t *Teamserver) AgentBuildLog(context map[string]any, format string, args ...any) {
	var (
		user string
		err  error
	)

	user = context["user"].(string)

	//
	// send the client the build log message
	//
	err = t.UserSend(user, t.EventCreate(EventAgentBuildLog, map[string]any{
		"log": fmt.Sprintf(format, args...),
	}))

	if err != nil {
		logger.DebugError("failed to send build log message to %v: %v", user, err)
		return
	}
}

func (t *Teamserver) AgentCallback(uuid string, data map[string]any) {
	t.UserBroadcast(false, t.EventCreate(EventAgentCallback, map[string]any{
		"uuid": uuid,
		"data": data,
	}))
}

func (t *Teamserver) AgentConsole(uuid string, data map[string]any) {
	var (
		serialize bytes.Buffer
		err       error
	)

	// encode the agent console data into a serialized
	// bytes buffer to be inserted into the database
	if err = gob.NewEncoder(&serialize).Encode(data); err == nil {
		// finally insert the serialize map into the database
		if err = t.database.AgentConsoleInsert(uuid, serialize.Bytes()); err != nil {
			logger.DebugError("database.AgentConsoleInsert encode error: %v", err)
		}
	} else {
		// we will just continue and broadcast the console message to all clients
		logger.DebugError("gob.NewEncoder(&serialize).Encode error: %v", err)
	}

	t.UserBroadcast(false, t.EventCreate(EventAgentConsole, map[string]any{
		"uuid": uuid,
		"data": data,
	}))
}

func (t *Teamserver) AgentHeartbeat(uuid, time string) {
	t.UserBroadcast(false, t.EventCreate(EventAgentHeartbeat, map[string]any{
		"uuid": uuid,
		"time": time,
	}))
}

func (t *Teamserver) AgentTypeExist(implant string) bool {
	for _, agent := range t.payloads {
		if agent.Data["name"].(string) == implant {
			return true
		}
	}

	return false
}

func (t *Teamserver) AgentProcessRequest(implant string, ctx map[string]any, request []byte) ([]byte, error) {
	if t.AgentTypeExist(implant) {
		return t.plugins.AgentProcess(implant, ctx, request)
	}

	return nil, errors.New("agent to process request not found")
}

// AgentGenerate
// TODO: change the function type to be AgentGenerate(implant, context, config)
func (t *Teamserver) AgentGenerate(ctx map[string]any, config map[string]any) (string, []byte, map[string]any, error) {
	if t.AgentTypeExist(ctx["name"].(string)) {
		return t.plugins.AgentGenerate(ctx, config)
	}

	return "", nil, nil, errors.New("agent to generate not found")
}

func (t *Teamserver) AgentExecute(uuid string, data map[string]any, wait bool) (map[string]any, error) {
	var (
		agent *Agent
		value any
		ok    bool
	)

	// load stored agent by uuid from map
	if value, ok = t.agents.Load(uuid); ok {
		agent = value.(*Agent)
		return t.plugins.AgentExecute(agent.plugin, uuid, data, wait)
	}

	return nil, errors.New("agent by uuid not found")
}

func (t *Teamserver) DatabaseAgentInsert(uuid, _type, parent, status, note string, metadata any) error {
	var (
		serialize bytes.Buffer
		err       error
	)

	// encode the metadata object into a serialized
	// bytes buffer to be inserted into the database
	if err = gob.NewEncoder(&serialize).Encode(metadata); err != nil {
		return err
	}

	// insert the agent into the database
	err = t.database.AgentInsert(uuid, _type, parent, status, note, serialize.Bytes())
	if err != nil {
		return err
	}

	return err
}

func (t *Teamserver) DatabaseAgentUpdate(uuid string, parent, status, note string, metadata any) error {
	var (
		serialize bytes.Buffer
		err       error
	)

	// encode the metadata object into a serialized
	// bytes buffer to be inserted into the database
	if err = gob.NewEncoder(&serialize).Encode(metadata); err != nil {
		return err
	}

	// insert the agent into the database
	err = t.database.AgentUpdate(uuid, parent, status, note, serialize.Bytes())
	if err != nil {
		return err
	}

	return err
}

func (t *Teamserver) DatabaseAgentExist(uuid string) (bool, error) {
	return t.database.AgentExist(uuid)
}

func (t *Teamserver) DatabaseAgentType(uuid string) (string, error) {
	return t.database.AgentType(uuid)
}

func (t *Teamserver) DatabaseAgentParent(uuid string) (string, error) {
	return t.database.AgentParent(uuid)
}

func (t *Teamserver) DatabaseAgentStatus(uuid string) (string, error) {
	return t.database.AgentStatus(uuid)
}

func (t *Teamserver) DatabaseAgentNote(uuid string) (string, error) {
	return t.database.AgentNote(uuid)
}

func (t *Teamserver) DatabaseAgentMetadata(uuid string) ([]byte, error) {
	return t.database.AgentMetadata(uuid)
}

func (t *Teamserver) DatabaseAgentHidden(uuid string) (bool, error) {
	return t.database.AgentHidden(uuid)
}

func (t *Teamserver) DatabaseAgentDisabled(uuid string) (bool, error) {
	return t.database.AgentDisabled(uuid)
}

func (t *Teamserver) DatabaseAgentSetStatus(uuid string, status string) error {
	return t.database.AgentSetStatus(uuid, status)
}

func (t *Teamserver) DatabaseAgentSetNote(uuid string, note string) error {
	return t.database.AgentSetNote(uuid, note)
}

func (t *Teamserver) DatabaseAgentSetDisabled(uuid string, disabled bool) error {
	return t.database.AgentSetDisabled(uuid, disabled)
}

func (t *Teamserver) DatabaseAgentSetHide(uuid string, hide bool) error {
	return t.database.AgentSetHide(uuid, hide)
}

func (t *Teamserver) DatabaseAgentConsole(uuid string) ([]map[string]any, error) {
	var (
		entries [][]byte
		console []map[string]any
		err     error
	)

	// get the entries from the database
	entries, err = t.database.AgentConsole(uuid)
	if err != nil {
		logger.DebugError(LogPrefix+"AgentConsole error on %v: %v", uuid, err)
		return nil, err
	}

	// iterate over the entries and decode each entry
	// into a map and add it to the console map array
	for _, entry := range entries {
		var data map[string]any

		err = gob.NewDecoder(bytes.NewBuffer(entry)).Decode(&data)
		if err != nil {
			logger.DebugError(LogPrefix+"AgentConsole decode error on %v: %v", uuid, err)
			return nil, err
		}

		console = append(console, map[string]any{
			"uuid": uuid,
			"data": data,
		})
	}

	return console, err
}
