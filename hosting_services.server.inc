<?php

/**
 * List all servers.
 */
function hosting_services_list_servers() {
  $servers = array();
  $server_result = db_query("SELECT hs.*, node.title AS title FROM {hosting_server} hs INNER JOIN {node} ON hs.nid = node.nid WHERE hs.status <> :status_deleted", array(':status_deleted' => HOSTING_SERVER_DELETED))->fetchAll();

  foreach ($server_result as $server) {
    $servers[] = $server;
  }

  return $servers;
}

/**
 * Create servers.
 */
function hosting_services_create_server($hostname, $data) {
  $server = hosting_services_save_server($hostname, $data);
  if (!empty($server->nid)) {
    hosting_add_task($server->nid, 'verify');
    return $server;
  }
  return FALSE;
}

/**
 * Update servers.
 */
function hosting_services_save_server($hostname, $data) {
  global $user;
  // Try to retrieve an existing server.
  $server = hosting_get_server_by_hostname(check_plain($hostname));
  if ($server) {
    $server = (object) array_merge((array) $server, $data);
  }
  else {
    if (!is_numeric($data['platform']) || !is_numeric($data['profile'])) {
      return FALSE;
    }
    $server = new stdClass();
    $server->type = 'server';
    $server->title = $server->hosting_name = check_plain($hostname);
//    $server->db_server = (is_numeric($data['db_server'])) ? $data['db_server'] : variable_get('hosting_default_db_server', 0);
//    $server->client = ((isset($data['client']) && is_numeric($data['client']))) ? $data['client'] : variable_get('hosting_default_client', 1);
    $server = (object) array_merge((array) $server, $data);
    $server->uid = $user->uid;

  }

  drupal_alter('hosting_services_server_presave', $server);

  $form_state = array();
  node_validate($server, array(), $form_state);

  $server->status = 1;
  node_save($server);

  return $server;
}

/**
 * Create tasks.
 */
function hosting_services_server_task($type, $nid) {
  global $user;
  $tasks = hosting_available_tasks('server');
  if (array_key_exists($type, $tasks)) {
    $task = hosting_add_task($nid, $type);
    return $task;
  }
  else {
    return FALSE;
  }
}

/**
 * Retrieves a site node based on the URL.
 *
 * @param string $hostname
 *   The URL of the site.
 * @param boolean $loaded_object
 *   Determines if a loaded site should be returned, or just the node ID.
 *   Defaults to TRUE.
 * @return
 *   If $loaded_object is TRUE, a fully loaded site node object.
 *   If $loaded_object is FALSE, the node ID of the site.
 *   In either case, if the site can't be found, FALSE is returned.
 * @todo
 *   Remove the db_query() and have only one EntityFieldQuery. The two
 *   different cases will be handled by adding different conditions/tags to it.
 *   A new tag will have to be added to match the title for aliases. These tags
 *   won't be necessary in Drupal 8 as JOINs are natively supported. See
 *   https://www.drupal.org/node/1882418 for details.
 */
function hosting_get_server_by_hostname($hostname, $loaded_object = TRUE) {
  // If the Aliases feature is enabled, try and get the site by its alias too
  if (hosting_feature('alias')) {
    $nid = db_query("SELECT n.nid
                       FROM {node} n
                       JOIN {hosting_server} h
                         ON n.nid = h.nid
                      WHERE (n.title = :title)
                        AND n.type = :type
                        AND NOT (h.status = :status)",
      array(
        ':title' => $hostname,
        ':alias' => $hostname,
        ':type' => 'site',
        ':status' => HOSTING_SERVER_DELETED,
      ))->fetchField();
  }
  else {
    // Get the list of node IDs whose title matches the name.
    $query = new EntityFieldQuery();
    $entities = $query->entityCondition('entity_type', 'node')
      ->propertyCondition('type', 'site')
      ->propertyCondition('title', $hostname)
      ->addTag('server_not_deleted')
      ->range(0, 1)
      ->execute();

    // Get the ID of the single node, if there is one.
    if (!empty($entities['node'])) {
      // Fetch the node ID from the list and return it.
      $nids = array_keys($entities['node']);
      $nid = array_shift($nids);
    }
    else {
      $nid = FALSE;
    }
  }
  if ($nid) {
    return $loaded_object ? node_load($nid) : $nid;
  }
  return FALSE;
}