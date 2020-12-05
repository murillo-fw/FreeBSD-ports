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
require_once("bind.inc");
require_once("captiveportal.inc");
require_once("config.inc");
require_once("functions.inc");
require_once("filter.inc");
require_once("globals.inc");

function captivealias_update_aliases() {
  global $g, $config;

  $a_aliases = &$config['aliases']['alias'];
  $a_cp = &$config['captiveportal'];
  $cpzone = $config['installedpackages']['captivealias']['cpzone'];
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
  /* Clear aliases that start with user_ */
  if (is_array($a_aliases)) {
    foreach ($a_aliases as &$alias) {
      if (strcmp("user_", substr($alias['name'], 0, 4)) == 0) { // strcmp may not be needed
        unset($alias['address']);
        $changed = true;
      }
    }
  }

  foreach ($cpdb as $cpent) {
    $create_new_alias = true;
    $ip = $cpent['ip'];
    $user = "user_" . $cpent['username'];
    /* Check if entry already exists */
    if (is_array($a_aliases)) {
      foreach ($a_aliases as &$alias) {
        if (strcmp($alias['name'], $user) == 0) {
          /* Username already exists... check if IP is contained */
          if (!stristr($alias['address'], $ip)) {
            /* IP is wrong, update it */
            $alias['address'] .= ' ' . $ip;
            $changed = true;
            $create_new_alias = false;
          } else {
            /* Username and IP match. */
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

  if ($changed) {
    write_config("Add CaptiveAlias aliases", true, false);
  }

  /* Reload Firewall if we have a change */
  if ($changed) {
    $retval = 0;
    $retval |= filter_configure();

    if ($retval == 0) {
      clear_subsystem_dirty('aliases');
    }
  }
  return $changed;

}

function captivealias_bind_get_acl_index($name) {
  global $config;
  foreach ($config['installedpackages']['bindacls']['config'] as $key => $val) {
    if ($val['name'] == $name) {
      return $key;
    }
  }
  return;
}

function captivealias_get_alias($name) {
  global $config;
  foreach ($config['aliases']['alias'] as $key => $val) {
    $aclname = $val['name'];
    if ($aclname == $name) {
      return explode(" ", $val['address']);
    }
  }
  return -1;
}

function captivealias_bind_create_acl($name, $desc) {
  global $config;
  $config['installedpackages']['bindacls']['config'][captivealias_bind_get_acl_index($name)] =
    array(
      "name" => $name,
      "description" => $desc,
      "row" => ""
  );
  write_config("Add captivealias BIND acl", true, true);
}

function captivealias_bind_sync() {
  global $config;
  foreach (captivealias_get_categories() as $catname) {
    $catacl = captivealias_get_category_acl($catname);
    captivealias_bind_create_acl($catname, $catname, array());
    $bindacl = captivealias_bind_get_acl_index($catname);
    foreach (captivealias_get_category_users($catname) as $user) {
      foreach (captivealias_get_alias("user_{$user}") as $addr) {
        $config['installedpackages']['bindacls']['config'][$bindacl]['row'][] = array(
          "value" => $addr,
          "description" => "user_{$user}",
        );
      }
    }
    if (($config['installedpackages']['bindacls']['config'][$bindacl]['row'][0] == "") && (sizeof($config['installedpackages']['bindacls']['config'][$bindacl]['row'] > 1))) {
      \array_splice($config['installedpackages']['bindacls']['config'][$bindacl]['row'], 0, 1);
    } else if (($config['installedpackages']['bindacls']['config'][$bindacl]['row'][0] == "") && (sizeof($config['installedpackages']['bindacls']['config'][$bindacl]['row'] == 1))) {
      unset($config['installedpackages']['bindacls']['config'][$bindacl]);
    }
  }
  write_config("Sync CaptiveAlias config with BIND", true, true);
  bind_sync();
}

function captivealias_del_config() {
  global $config;
  unset($config['installedpackages']['captivealias']);
  write_config("CaptiveAlias: delete configuration", true, true);
}

function captivealias_init_config() {
  global $config;
  $config['installedpackages']['captivealias'] = array(
    'enable_captivealias' => "false",
    'cpzone' => "lan",
    'defaultcategory' => "family",
  );
  write_config('CaptiveAlias: init config', true, true);
}

function captivealias_get_enabled() {
  global $config;
  return($config['installedpackages']['captivealias']['enable_captivealias'] == "true");
}

function captivealias_set_enable() {
  global $config;
  if ($config['installedpackages']['captivealias']['enable_captivealias'] != "true") {
    $config['installedpackages']['captivealias']['enable_captivealias'] = "true";
  }
}

function captivealias_set_disable() {
  global $config;
  if ($config['installedpackages']['captivealias']['enable_captivealias'] != "false") {
    $config['installedpackages']['captivealias']['enable_captivealias'] = "false";
  }
}

function captivealias_set_defaultcategory($catname) {
  global $config;
  $config['installedpackages']['captivealias']['defaultcategory'] = htmlspecialchars($catname);
  write_config("CaptiveAlias: set defaultcategory to {$catname}", true, true);
}

function captivealias_get_defaultcategory() {
  global $config;
  return $config['installedpackages']['captivealias']['defaultcategory'];
}

function captivealias_get_cpzone() {
  global $config;
  return($config['installedpackages']['captivealias']['cpzone']);
}

function captivealias_set_cpzone($cpzone) {
  global $config;
  $config['installedpackages']['captivealias']['cpzone'] = htmlspecialchars($cpzone);
  write_config("CaptiveAlias: set cpzone to {$cpzone}", true, true);
}

function captivealias_add_category($catname) {
  global $config;
  if (!is_array($config['installedpackages']['captivealias']['cats'])) {
    $config['installedpackages']['captivealias']['cats'] = array();
  }
  $newcat = array(
    "name" => htmlspecialchars($catname)
  );

  $config['installedpackages']['captivealias']['cats'][htmlspecialchars($catname)] = $newcat;
  write_config("CaptiveAlias: add category {$catname}", true, true);
}

function captivealias_del_category($catname) {
  global $config;
  unset($config['installedpackages']['captivealias']['cats'][htmlspecialchars($catname)]);
  write_config("CaptiveAlias: delete category {$catname}", true, true);
}

function captivealias_get_categories() {
  global $config;
  $output = array();
  foreach ($config['installedpackages']['captivealias']['cats'] as $key => $val) {
    $output[] = $val['name'];
  }
  return $output;
}

function captivealias_set_category_acl($catname, $aclname) {
  global $config;
  $config['installedpackages']['captivealias']['cats'][htmlspecialchars($catname)]['acl'] = 
    htmlspecialchars($aclname);
  write_config("CaptiveAlias: set category {$catname} acl to {$aclname}", true, true);
}

function captivealias_get_category_acl($catname) {
  global $config;
  return($config['installedpackages']['captivealias']['cats'][htmlspecialchars($catname)]['acl']);
}

function captivealias_add_category_user($catname, $user) {
  global $config;
  if (!is_array($config['installedpackages']['captivealias']['cats'][htmlspecialchars($catname)]['devices'])) { 
    $config['installedpackages']['captivealias']['cats'][htmlspecialchars($catname)]['devices'] =
      array();
  }
  // We have to set the key to a non-numerical value to prevent some issue with pfSense
  $config['installedpackages']['captivealias']['cats'][htmlspecialchars($catname)]['devices'][htmlspecialchars($user)] =
    htmlspecialchars($user);
  write_config("CaptiveAlias: added user {$user} to category {$category}", true, true);
}

function captivealias_del_category_user($catname, $user) {
  global $config;
  unset($config['installedpackages']['captivealias']['cats'][htmlspecialchars($catname)]['devices'][htmlspecialchars($user)]);
  write_config("CaptiveAlias: delete user {$user} from category {$category}", true, true);
}

function captivealias_get_category_users($catname) {
  global $config;
  $output = array();
  foreach($config['installedpackages']['captivealias']['cats'][htmlspecialchars($catname)]['devices'] as $key => $val) {
    $output[] = $val;
  }
  return $output;
}

function captivealias_cron_sync() {
  if (captivealias_get_enabled()) {
    if (captivealias_update_aliases()) {
      captivealias_bind_sync();
    }
  }
}

?>
