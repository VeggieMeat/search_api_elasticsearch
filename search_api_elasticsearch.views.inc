<?php

/**
 * Implements hook_views_data_alter().
 */
function search_api_elasticsearch_views_data_alter(&$data) {
  foreach (search_api_server_load_multiple(FALSE, array('class' => 'search_api_elasticsearch_service'), TRUE) as $server) {
    foreach (search_api_index_load_multiple(FALSE, array('server' => $server->machine_name), TRUE) as $index) {
      $key = 'search_api_index_' . $index->machine_name;
      $table = &$data[$key];
      foreach ($table as &$info) {
        if (isset($info['field']) && is_array($info['field']) && $info['field']['type'] === 'date') {
          $info['field']['handler'] = 'SearchApiElasticsearchViewsHandlerFieldDate';
        }
      }
    }
  }
}