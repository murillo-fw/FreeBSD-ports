<?php
/*
 * captivealias.inc
 * part of Murillo (https://murillo.saggis.com)
 * Copyright (c) 2020 Saggi, LLC
 * All rights reserved
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* include all configuration functions */
require_once("captiveportal.inc");
require_once("config.inc");
require_once("functions.inc");
require_once("filter.inc");
require_once("globals.inc");

function captivealias_update_aliases() {
  global $g, $config;

  $a_aliases = &$config['aliases']['alias'];
  $a_cp = &$config['captiveportal'];
  $cpzone = "lan";
  $cpzoneid = $a_cp[$cpzone]['zoneid'];
  $db_path = "{$g['vardb_path']}/captiveportal{$cpzone}.db";
  $createquery = "CREATE TABLE IF NOT EXISTS captiveportal (" .
        "allow_time INTEGER, pipeno INTEGER, ip TEXT, mac TEXT, username TEXT, " .
        "sessionid TEXT, bpassword TEXT, session_timeout INTEGER, idle_timeout INTEGER, " .
        "session_terminate_time INTEGER, interim_interval INTEGER, traffic_quota INTEGER, " .
        "authmethod TEXT, context TEXT); " .
      "CREATE UNIQUE INDEX IF NOT EXISTS idx_active ON captiveportal (sessionid, username); " .
      "CREATE INDEX IF NOT EXISTS user ON captiveportal (username); " .
      "CREATE INDEX IF NOT EXISTS ip ON captiveportal (ip); " .
      "CREATE INDEX IF NOT EXISTS starttime ON captiveportal (allow_time)";

  try {
    $DB = new SQLite3($db_path);
    $DB->busyTimeout(60000);
  } catch (Exception $e) {
    captiveportal_syslog("Could not open {$db_path} as an sqlite database for {$cpzone}. Error message: " . $e->getMessage() . " -- Trying again.");
    unlink_if_exists($db_path);
    try {
      $DB = new SQLite3($db_path);
      $DB->busyTimeout(60000);
    } catch (Exception $e) {
      captiveportal_syslog("Still could not open {$db_path} as an sqlite database for {$cpzone}. Error message: " . $e->getMessage() . " -- Remove the database file manually and ensure there is enough free space.");
      return;
    }
  }

  if (!$DB) {
    captiveportal_syslog("Could not open {$db_path} as an sqlite database for {$cpzone}. Error message: {$DB->lastErrorMsg()}. Trying again.");
    unlink_if_exists($db_path);
    $DB = new SQLite3($db_path);
    $DB->busyTimeout(60000);
    if (!$DB) {
      captiveportal_syslog("Still could not open {$db_path} as an sqlite database for {$cpzone}. Error message: {$DB->lastErrorMsg()}. Remove the database file manually and ensure there is enough free space.");
      return;
    }
  }

  if (! $DB->exec($createquery)) {
    captiveportal_syslog("Error during table {$cpzone} creation. Error message: {$DB->lastErrorMsg()}. Resetting and trying again.");

    /* If unable to initialize the database, reset and try again. */
    $DB->close();
    unset($DB);
    unlink_if_exists($db_path);
    $DB = new SQLite3($db_path);
    $DB->busyTimeout(60000);
    if ($DB->exec($createquery)) {
      captiveportal_syslog("Successfully reinitialized tables for {$cpzone} -- database has been reset.");
      if (!is_numericint($cpzoneid)) {
        if (is_array($config['captiveportal'])) {
          foreach ($config['captiveportal'] as $cpkey => $cp) {
            if ($cpzone == $cpkey) {
              $cpzoneid = $cp['zoneid'];
            }
          }
        }
      }
      if (is_numericint($cpzoneid)) {
        $table_names = captiveportal_get_ipfw_table_names();
        foreach ($table_names as $table_name) {
          mwexec("/sbin/ipfw table {$table_name} flush");
        }
        captiveportal_syslog("Flushed tables for {$cpzone} after database reset.");
      }
    } else {
      captiveportal_syslog("Still unable to create tables for {$cpzone}. Error message: {$DB->lastErrorMsg()}. Remove the database file manually and try again.");
    }
  }
  $cpdb = array();
  if ($DB) {
    $response = $DB->query("SELECT * FROM captiveportal {$query}");
    if ($response != FALSE) {
      while ($row = $response->fetchArray()) {
        $cpdb[] = $row;
      }
    }
    $DB->close();
  }
  $changed = false;
  foreach ($cpdb as $cpent) {
    $create_new_alias = true;
    $ip = $cpent['ip'];
    $user = "user_" . $cpent['username'];
    /* Check if entry already exists */
    if (is_array($a_aliases)) {
      foreach ($a_aliases as &$alias) {
        print_r($alias);
        if ($alias['name'] == $user) {
          /* Username already exists... */
          print_r("Username already exists.\n");
          if (!stristr($alias['address'], $ip)) {
            /* IP is wrong, update it */
            print_r("IP is not included in alias.\n");
            print_r("Current IP is " . $alias['address'] . "\n");
            $alias['address'] .= ' ' . $ip;
            print_r("New IP is " . $alias['address'] . "\n");
            $changed = true;
            $create_new_alias = false;
          } else {
            /* Username and IP match. */
            print_r("Username and IP(s) match\n");
            $create_new_alias = false;
          }
        }
      }
    }
    if ($create_new_alias) {
      $a_aliases[] = array(
        'name' => $user,
        'address' => $ip,
        'type' => 'host',
        'detail' => 'DO NOT EDIT THIS ALIAS',
        'descr' => 'Added by CaptiveAlias'
      );
      $changed = true;
    }
  }
  write_config();

  /* Reload Firewall if we have a change */
  if ($changed) {
    $retval = 0;
    $retval |= filter_configure();

    if ($retval == 0) {
      clear_subsystem_dirty('aliases');
    }
  }
}

function captivealias_enabled() {
  global $config, $aliasservice_enabled;
  $aliasservice_enabled = false;
  if ($config['installedpackages']['captivealias'][0]['enable_captivealias'] == "on") {
    $aliasservice_enabled = true;
  }
  return $aliasservice_enabled;
}
?>
