<?php

/*
 * e2guardian.priv.inc
 *
 * part of Murillo (https://murillo.saggis.com)
 * Copyright (c) 2020 Saggi, LLC
 * All rights reserved
 * 
 * originally from pfSense (https://www.pfsense.org)
 * Copyright (c) 2017 Marcello Coutinho
 * Copyright (c) 2020 Rubicon Communications, LLC (Netgate)
 * All rights reserved.
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

global $priv_list;

$priv_list['page-services-e2guardian'] = array();
$priv_list['page-services-e2guardian']['name'] = "WebCfg -Services: E2guardian package";
$priv_list['page-services-e2guardian']['descr'] = "Allow access to E2guardian package GUI";

$priv_list['page-services-e2guardian']['match'] = array();
$priv_list['page-services-e2guardian']['match'][] = "pkg.php?xml=e2guardian*";
$priv_list['page-services-e2guradian']['match'][] = "pkg_edit.php?xml=e2guardian*";
$priv_list['page-services-e2guradian']['match'][] = "e2guardian_about.php";
$priv_list['page-services-e2guradian']['match'][] = "e2guardian_monitor.php";
$priv_list['page-services-e2guradian']['match'][] = "e2guardian_monitor_data.php";
$priv_list['page-services-e2guradian']['match'][] = "e2guardian_log_parser.php";
$priv_list['page-services-e2guradian']['match'][] = "e2guardian.widget.php";

?>
