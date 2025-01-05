package server

import (
	"Havoc/pkg/db"
	"encoding/gob"
	"os"

	"Havoc/pkg/api"
	"Havoc/pkg/colors"
	"Havoc/pkg/logger"
	"Havoc/pkg/plugin"
	"Havoc/pkg/profile"
)

var (
	Version      = "1.0"
	CodeName     = "King Of The Damned"
	DatabasePath = "/database.db"
)

func init() {
	gob.Register(map[string]any{})
	gob.Register([]any{})
}

func NewTeamserver() *Teamserver {
	var (
		server = new(Teamserver)
		err    error
	)

	if err = server.createConfigPath(); err != nil {
		logger.Error("couldn't create config folder: %v", err.Error())
		return nil
	}

	return server
}

func (t *Teamserver) SetFlags(flags Flags) {
	t.flags = flags
}

func (t *Teamserver) Start() {
	var (
		err               error
		server            profile.Server
		certPath, keyPath string
		plugins           []db.Plugin
	)

	// initialize database that is going to store the agent connections, listener status/logs, etc.
	if t.database, err = db.NewDatabase(t.ConfigPath() + DatabasePath); err != nil {
		logger.Error("failed to initialize database: " + colors.Red(err.Error()))
		return
	}

	// create a server api endpoints
	if t.server, err = api.NewServerApi(t); err != nil {
		logger.Error("failed to initialize api server: " + colors.Red(err.Error()))
		return
	}

	// generate a new plugin system instance
	t.plugins = plugin.NewPluginSystem(t)

	// parse the provided server profile
	server, err = t.profile.Server()
	if err != nil {
		logger.Error("failed to parse profile server block: " + err.Error())
		return
	}

	// load all registered plugins from the database
	if plugins, err = t.database.PluginList(); err == nil {
		for _, entry := range plugins {
			var ext *plugin.Plugin

			if ext, err = t.plugins.RegisterPlugin(entry.Path); err != nil {
				logger.Info("%s failed to load plugin %s: %v", colors.BoldBlue("[plugin]"), entry.Path, colors.Red(err))
			}

			if ext != nil {
				logger.Info("%s plugin loaded: %v (%v)", colors.BoldBlue("-"), colors.BoldBlue(ext.Name), ext.Type)
			}
		}
	} else {
		logger.Error("failed to load plugin list: " + err.Error())
	}

	// load all plugins that has been specified in the profile configuration
	if len(server.Plugins) != 0 {
		for i := range server.Plugins {
			var ext *plugin.Plugin

			// we also have to check if the plugin has been
			// already loaded previously from the database
			if ext, err = t.plugins.RegisterPlugin(server.Plugins[i]); err != nil && !plugin.IsAlreadyRegistered(err) {
				logger.Info("%s failed to load plugin %s: %v", colors.BoldBlue("[plugin]"), server.Plugins[i], colors.Red(err))
			}

			if ext != nil {
				logger.Info("%s plugin loaded: %v (%v)", colors.BoldBlue("-"), colors.BoldBlue(ext.Name), ext.Type)
			}
		}
	}

	// check if the ssl certification has been set in the profile
	if len(server.Ssl.Key) == 0 && len(server.Ssl.Key) == 0 {
		// has not been set, so we are going to generate a new pair of certs
		certPath, keyPath, err = t.server.GenerateSSL(server.Host, t.ConfigPath())
		if err != nil {
			return
		}

		logger.Info("%v ssl cert: %v", colors.BoldGreen("[auto]"), certPath)
		err = t.server.SetSSL(certPath, keyPath)
		if err != nil {
			logger.Error("failed to set ssl cert: %v", colors.Red(err))
			return
		}
	} else {
		logger.Info("%v ssl cert: %v", colors.BoldYellow("[custom]"), server.Ssl.Cert)
		err = t.server.SetSSL(server.Ssl.Cert, server.Ssl.Key)
		if err != nil {
			logger.Error("failed to set ssl cert: %v", colors.Red(err))
			return
		}
	}

	// restore the agent sessions from the database
	if err = t.RestoreListeners(); err != nil {
		logger.Error("failed to restore listeners from database: %v", colors.Red(err))
		return
	}

	// restore the agent sessions from the database
	if err = t.RestoreAgents(); err != nil {
		logger.Error("failed to restore agents from database: %v", colors.Red(err))
		return
	}

	// finally start the api endpoints and our
	// teamserver for the clients to interact with
	t.server.Start(server.Host, server.Port)
}

// Version
// get the current server version
func (*Teamserver) Version() map[string]string {
	return map[string]string{
		"version":  Version,
		"codename": CodeName,
	}
}

func (t *Teamserver) Config() map[string]any {
	return t.profile.Config()
}

func (t *Teamserver) createConfigPath() error {
	var err error

	if t.configPath, err = os.UserHomeDir(); err != nil {
		return err
	}

	t.configPath += "/.havoc/server"

	if err = os.MkdirAll(t.configPath, os.ModePerm); err != nil {
		return err
	}

	// create listeners directory
	if err = os.MkdirAll(t.configPath+"/listeners", os.ModePerm); err != nil {
		return err
	}

	// create agents directory
	if err = os.MkdirAll(t.configPath+"/agents", os.ModePerm); err != nil {
		return err
	}

	return nil
}

func (t *Teamserver) ConfigPath() string {
	return t.configPath
}

func (t *Teamserver) Profile(path string) error {
	var err error

	t.profile = profile.NewProfile()

	if err = t.profile.Parse(path); err != nil {
		logger.SetStdOut(os.Stderr)
		logger.Error("profile parsing error: %v", colors.Red(err))
		return err
	}

	return nil
}

func (t *Teamserver) PluginLoad(name string) (any, error) {
	return t.plugins.Plugin(name)
}
