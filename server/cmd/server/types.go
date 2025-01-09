package server

import (
	"Havoc/pkg"
	"Havoc/pkg/api"
	"Havoc/pkg/db"
	"Havoc/pkg/plugin"
	"Havoc/pkg/profile"
	"sync"

	"github.com/gorilla/websocket"
)

type HavocUser struct {
	username string
	mutex    sync.Mutex
	socket   *websocket.Conn
}

type Flags struct {
	Profile string
	Debug   bool
	Default bool
}

type Handler struct {
	Name string         `json:"name"`
	Data map[string]any `json:"data"`
}

type Listener struct {
	Name     string `json:"name"`
	Protocol string `json:"protocol"`
	Status   string `json:"status"`
	Host     string `json:"host"`
	Port     string `json:"port"`
	Path     string `json:"path"`
}

const (
	AgentCommandProcess = iota
	AgentCommandPivotPoke
)

type AgentCommand struct {
	Type    int
	Command func(uuid string, context any, data []byte) (bool, error)
}

type Agent struct {
	uuid string

	// plugin is agent type
	plugin string

	// status of agent which can be custom such as: healthy, timeout, etc.
	// the status string can be prefixed with an + (green), - (red), and * (cyan)
	status string

	// note of agent
	note string
}

type Teamserver struct {
	flags  Flags
	server *api.ServerApi

	database   *db.Database
	profile    *profile.Profile
	configPath string
	events     struct {
		mutex sync.Mutex
		list  []map[string]any
	}

	clients sync.Map
	plugins *plugin.System

	// registered protocol types and running listeners
	protocols []Handler  // available handlers/listeners to use
	listener  []Listener // started listeners

	// registered implant types and agent sessions
	payloads []Handler     // available payload types
	agents   pkg.HcSyncMap // current connected agent sessions

	// registered agent commands by external plugins and protocols
	// which allows to extend the implants commands to add additional
	// features and ways to process callbacks
	//
	// map[string][]func([]byte) (bool, error)
	commands sync.Map
}
