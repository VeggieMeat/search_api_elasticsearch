<?php

  /**
   * Contains base test class for Search API Elasticsearch.
   */

class SearchApiElasticsearchBaseTest extends \PHPUnit_Framework_TestCase {

  /**
   * Create Search API server.
   */
  protected function _createServer($name = 'test', $class, $options = array()) {
    return $this->_server = entity_create('search_api_server', array(
      'name' => $name,
      'machine name' => $name,
      'class' => $class,
      'options' => $options,
      'enabled' => 1,
      'status' => 1,
    ));
  }

  /**
   * Create Search API index.
   */
  protected function _createIndex($name = 'test', $type, $server) {
    return $this->_index = entity_create('search_api_index', array(
      'name' => $name,
      'machine name' => $name,
      'enabled' => 1,
      'item type' => $type,
      'server' => $server,
    ));
  }

}
