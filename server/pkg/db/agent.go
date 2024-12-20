package db

import (
	"Havoc/pkg/logger"
	"database/sql"
	"errors"
)

type Agent struct {
	Uuid     string
	Type     string
	Parent   string
	Status   string
	Note     string
	Metadata []byte
}

func (db *Database) AgentInsert(uuid, _type, parent, status, note string, metadata []byte) error {
	var (
		stmt  *sql.Stmt
		err   error
		exist bool
	)

	// check if the to be inserted agent uuid already exists
	exist, err = db.AgentExists(uuid)
	if err != nil {
		return err
	} else if exist {
		return errors.New("agent already exist")
	}

	// create sql insert statement
	if stmt, err = db.sqlite.Prepare(`
        INSERT INTO Agents (uuid, type, parent, status, note, metadata, disabled) values(?, ?, ?, ?, ?, ?, ?)
    `); err != nil {
		logger.DebugError("sqlite.Prepare failed: %v", err)
		return err
	}

	// insert the data into the created sql statement
	if _, err = stmt.Exec(
		uuid,
		_type,
		parent,
		status,
		note,
		metadata,
		false,
	); err != nil {
		logger.DebugError("stmt.Exec failed: %v", err)
		return err
	}

	return err
}

func (db *Database) AgentUpdate(uuid, parent, status, note string, metadata []byte) error {
	var (
		stmt  *sql.Stmt
		err   error
		exist bool
	)

	if exist, err = db.AgentExists(uuid); err != nil {
		return err
	} else if !exist {
		return errors.New("agent not exist")
	}

	// create sql insert statement
	if stmt, err = db.sqlite.Prepare(`
        UPDATE Agents SET parent = ?, status = ?, note = ?, metadata = ? WHERE uuid = ?
    `); err != nil {
		logger.DebugError("sqlite.Prepare failed: %v", err)
		return err
	}

	// insert the data into the created sql statement
	if _, err = stmt.Exec(parent, status, note, metadata, uuid); err != nil {
		logger.DebugError("stmt.Exec failed: %v", err)
		return err
	}

	return err
}

func (db *Database) AgentExists(uuid string) (bool, error) {
	var (
		stmt *sql.Stmt
		err  error
	)

	// prepare statement for the sql query
	stmt, err = db.sqlite.Prepare("SELECT COUNT(*) FROM Agents WHERE uuid = ?")
	if err != nil {
		logger.DebugError("sqlite.Prepare failed: %v", err)
		return false, err
	}

	// execute statement
	query, err := stmt.Query(uuid)
	defer query.Close()
	if err != nil {
		logger.DebugError("stmt.Query failed: %v", err)
		return false, err
	}

	for query.Next() {
		var NumRows int

		err = query.Scan(&NumRows)
		if err != nil {
			logger.DebugError("query.Scan failed: %v", err)
			return false, err
		}

		if NumRows >= 1 {
			return true, err
		} else {
			return false, err
		}
	}

	return false, nil
}

func (db *Database) AgentMetadata(uuid string) ([]byte, error) {
	var (
		stmt     *sql.Stmt
		err      error
		metadata []byte
		exist    bool
	)

	if exist, err = db.AgentExists(uuid); err != nil {
		return nil, err
	} else if !exist {
		return nil, errors.New("agent not exist")
	}

	stmt, err = db.sqlite.Prepare("SELECT metadata FROM Agents WHERE uuid = ?")
	if err != nil {
		logger.DebugError("sqlite.Prepare failed: %v", err)
		return nil, err
	}

	err = stmt.QueryRow(uuid).Scan(&metadata)
	if err != nil {
		logger.DebugError("stmt.Query failed: %v", err)
		return nil, err
	}

	return metadata, err
}

func (db *Database) AgentType(uuid string) (string, error) {
	var (
		stmt  *sql.Stmt
		err   error
		_type string
		exist bool
	)

	if exist, err = db.AgentExists(uuid); err != nil {
		return "", err
	} else if !exist {
		return "", errors.New("agent not exist")
	}

	stmt, err = db.sqlite.Prepare("SELECT type FROM Agents WHERE uuid = ?")
	if err != nil {
		logger.DebugError("sqlite.Prepare failed: %v", err)
		return "", err
	}

	err = stmt.QueryRow(uuid).Scan(&_type)
	if err != nil {
		logger.DebugError("stmt.Query failed: %v", err)
		return "", err
	}

	return _type, err
}

func (db *Database) AgentParent(uuid string) (string, error) {
	var (
		stmt   *sql.Stmt
		err    error
		parent string
		exist  bool
	)

	if exist, err = db.AgentExists(uuid); err != nil {
		return "", err
	} else if !exist {
		return "", errors.New("agent not exist")
	}

	stmt, err = db.sqlite.Prepare("SELECT parent FROM Agents WHERE uuid = ?")
	if err != nil {
		logger.DebugError("sqlite.Prepare failed: %v", err)
		return "", err
	}

	err = stmt.QueryRow(uuid).Scan(&parent)
	if err != nil {
		logger.DebugError("stmt.Query failed: %v", err)
		return "", err
	}

	return parent, err
}

func (db *Database) AgentStatus(uuid string) (string, error) {
	var (
		stmt   *sql.Stmt
		err    error
		status string
		exist  bool
	)

	if exist, err = db.AgentExists(uuid); err != nil {
		return "", err
	} else if !exist {
		return "", errors.New("agent not exist")
	}

	stmt, err = db.sqlite.Prepare("SELECT status FROM Agents WHERE uuid = ?")
	if err != nil {
		logger.DebugError("sqlite.Prepare failed: %v", err)
		return "", err
	}

	err = stmt.QueryRow(uuid).Scan(&status)
	if err != nil {
		logger.DebugError("stmt.Query failed: %v", err)
		return "", err
	}

	return status, err
}

func (db *Database) AgentNote(uuid string) (string, error) {
	var (
		stmt  *sql.Stmt
		err   error
		note  string
		exist bool
	)

	if exist, err = db.AgentExists(uuid); err != nil {
		return "", err
	} else if !exist {
		return "", errors.New("agent not exist")
	}

	stmt, err = db.sqlite.Prepare("SELECT note FROM Agents WHERE uuid = ?")
	if err != nil {
		logger.DebugError("sqlite.Prepare failed: %v", err)
		return "", err
	}

	err = stmt.QueryRow(uuid).Scan(&note)
	if err != nil {
		logger.DebugError("stmt.Query failed: %v", err)
		return "", err
	}

	return note, err
}

func (db *Database) AgentDisabled(uuid string) (bool, error) {
	var (
		stmt     *sql.Stmt
		err      error
		disabled bool
		exist    bool
	)

	if exist, err = db.AgentExists(uuid); err != nil {
		return false, err
	} else if !exist {
		return false, errors.New("agent not exist")
	}

	stmt, err = db.sqlite.Prepare("SELECT disabled FROM Agents WHERE uuid = ?")
	if err != nil {
		logger.DebugError("sqlite.Prepare failed: %v", err)
		return false, err
	}

	err = stmt.QueryRow(uuid).Scan(&disabled)
	if err != nil {
		logger.DebugError("stmt.Query failed: %v", err)
		return false, err
	}

	return false, err
}

func (db *Database) AgentSetStatus(uuid string, status string) error {
	var (
		stmt  *sql.Stmt
		err   error
		exist bool
	)

	if exist, err = db.AgentExists(uuid); err != nil {
		return err
	} else if !exist {
		return errors.New("agent not exist")
	}

	// create sql insert statement
	if stmt, err = db.sqlite.Prepare(`
        UPDATE Agents SET status = ? WHERE uuid = ?
    `); err != nil {
		logger.DebugError("sqlite.Prepare failed: %v", err)
		return err
	}

	// insert the data into the created sql statement
	if _, err = stmt.Exec(status, uuid); err != nil {
		logger.DebugError("stmt.Exec failed: %v", err)
		return err
	}

	return err
}

func (db *Database) AgentSetNote(uuid string, note string) error {
	var (
		stmt  *sql.Stmt
		err   error
		exist bool
	)

	if exist, err = db.AgentExists(uuid); err != nil {
		return err
	} else if !exist {
		return errors.New("agent not exist")
	}

	// create sql insert statement
	if stmt, err = db.sqlite.Prepare(`
        UPDATE Agents SET note = ? WHERE uuid = ?
    `); err != nil {
		logger.DebugError("sqlite.Prepare failed: %v", err)
		return err
	}

	// insert the data into the created sql statement
	if _, err = stmt.Exec(note, uuid); err != nil {
		logger.DebugError("stmt.Exec failed: %v", err)
		return err
	}

	return err
}

func (db *Database) AgentSetDisabled(uuid string, disabled bool) error {
	var (
		stmt  *sql.Stmt
		err   error
		exist bool
	)

	if exist, err = db.AgentExists(uuid); err != nil {
		return err
	} else if !exist {
		return errors.New("agent not exist")
	}

	// create sql insert statement
	if stmt, err = db.sqlite.Prepare(`
        UPDATE Agents SET disabled = ? WHERE uuid = ?
    `); err != nil {
		logger.DebugError("sqlite.Prepare failed: %v", err)
		return err
	}

	// insert the data into the created sql statement
	if _, err = stmt.Exec(disabled, uuid); err != nil {
		logger.DebugError("stmt.Exec failed: %v", err)
		return err
	}

	return err
}

func (db *Database) AgentList() ([]Agent, error) {
	var (
		agents []Agent
		query  *sql.Rows
		err    error
	)

	// prepare the query statement
	query, err = db.sqlite.Query("SELECT uuid, type, parent, status, note, metadata FROM Agents WHERE disabled = 0")
	if err != nil {
		logger.DebugError("sqlite.Query failed: %v", err)
		return nil, err
	}

	defer query.Close()

	// iterate over the list of agents and
	// scan them into the agent entry structure
	for query.Next() {
		var entry Agent

		err = query.Scan(&entry.Uuid, &entry.Type, &entry.Parent, &entry.Status, &entry.Note, &entry.Metadata)
		if err != nil {
			logger.DebugError("query.Scan failed: %v", err)
			return nil, err
		}

		agents = append(agents, entry)
	}

	return agents, nil
}

func (db *Database) AgentConsole(uuid string) ([][]byte, error) {
	var (
		entry []byte
		list  [][]byte
		query *sql.Rows
		err   error
		exist bool
	)

	if exist, err = db.AgentExists(uuid); err != nil {
		return nil, err
	} else if !exist {
		return nil, errors.New("agent not exist")
	}

	query, err = db.sqlite.Query("SELECT data FROM AgentConsole WHERE uuid = ?", uuid)
	if err != nil {
		logger.DebugError("sqlite.Query failed: %v", err)
		return nil, err
	}

	defer query.Close()

	for query.Next() {
		if err = query.Scan(&entry); err != nil {
			logger.DebugError("query.Scan failed: %v", err)
			return nil, err
		}

		list = append(list, entry)
	}

	return list, nil
}

func (db *Database) AgentRemove(uuid string) error {
	var (
		stmt  *sql.Stmt
		err   error
		exist bool
	)

	if exist, err = db.AgentExists(uuid); err != nil {
		return err
	} else if !exist {
		return errors.New("agent not exist")
	}

	// create sql delete statement
	if stmt, err = db.sqlite.Prepare(`
        DELETE FROM Agents WHERE uuid = ?
    `); err != nil {
		logger.DebugError("sqlite.Prepare failed: %v", err)
		return err
	}

	// insert the data into the created sql statement
	if _, err = stmt.Exec(uuid); err != nil {
		logger.DebugError("stmt.Exec failed: %v", err)
		return err
	}

	return err
}

func (db *Database) AgentConsoleInsert(uuid string, data []byte) error {
	var (
		stmt  *sql.Stmt
		err   error
		exist bool
	)

	if exist, err = db.AgentExists(uuid); err != nil {
		return err
	} else if !exist {
		return errors.New("agent not exist")
	}

	// create sql insert statement
	if stmt, err = db.sqlite.Prepare(`
        INSERT INTO AgentConsole (uuid, data) values(?, ?)
    `); err != nil {
		logger.DebugError("sqlite.Prepare failed: %v", err)
		return err
	}

	// insert the data into the created sql statement
	if _, err = stmt.Exec(uuid, data); err != nil {
		logger.DebugError("stmt.Exec failed: %v", err)
		return err
	}

	return err
}
