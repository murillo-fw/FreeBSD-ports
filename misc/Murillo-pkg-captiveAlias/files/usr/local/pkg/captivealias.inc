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
require_once("auth.inc");
require_once("config.inc");
require_once("guiconfig.inc");
require_once("functions.inc");
require_once("filter.inc");
require_once("captiveportal.inc");

$a_aliases = &$config['aliases']['alias'];

function captivealias_update_aliases() {
  $changed = false;
  $cpdb = captiveportal_read_db();
  foreach ($cpdb as $cpent) {
    $ip = $cpent[2];
    $user = "user_" . $cpent[4];
    if (!($a_aliases[$user] == $ip)) {
      $a_aliases[$user] = $ip;
      $changed = true;
    }
  }

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
