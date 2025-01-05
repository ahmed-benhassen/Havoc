package pkg

import "github.com/gorilla/websocket"

type IHavocCore interface {
	LogInfo(fmt string, args ...any)
	LogError(fmt string, args ...any)
	LogWarn(fmt string, args ...any)
	LogDebug(fmt string, args ...any)
	LogDbgError(fmt string, args ...any)
	LogFatal(fmt string, args ...any)
	LogPanic(fmt string, args ...any)

	Version() map[string]string
	Config() map[string]any

	UserAuthenticate(username, password string) bool
	UserLogin(token string, login any, socket *websocket.Conn)
	UserLogoutByToken(user string) error
	UserNameByToken(token string) (string, error)
	UserStatus(username string) int

	ListenerStart(name, protocol string, options map[string]any) error
	ListenerStop(name string) error
	ListenerRestart(name string) error
	ListenerRemove(name string) error
	ListenerEvent(protocol string, event map[string]any) (map[string]any, error)
	ListenerConfig(name string) (map[string]any, error)
	ListenerEdit(name string, config map[string]any) error
	ListenerList() []map[string]string
	ListenerRegisterType(name string, listener map[string]any) error
	ListenerProtocol(name string) (string, error)

	AgentInitialize(uuid, plugin, status, note string, data map[string]any) error
	AgentData(uuid string) (map[string]any, error)
	AgentExists(uuid string) bool
	AgentType(uuid string) (string, error)
	AgentRemove(uuid string) error
	AgentList() []string
	AgentGenerate(implant string, config map[string]any) (string, []byte, map[string]any, error)
	AgentExecute(uuid string, data map[string]any, wait bool) (map[string]any, error)
	AgentNote(uuid string) (string, error)
	AgentSetNote(uuid string, note string) error
	AgentStatus(uuid string) (string, error)
	AgentSetStatus(uuid string, status string) error
	AgentRegisterType(name string, agent map[string]any) error

	PluginList() []map[string]any
	PluginResource(name, resource string) ([]byte, error)

	DatabaseAgentConsole(uuid string) ([]map[string]any, error)
}
