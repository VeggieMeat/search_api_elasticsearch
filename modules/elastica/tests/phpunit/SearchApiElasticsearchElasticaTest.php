<?php

  /**
   * @file
   * Contains tests for SearchApiElasticsearchElastica.
   */

class SearchApiElasticsearchElasticaTest extends SearchApiElasticsearchBaseTest {

  public function setUp() {
    $this->_server = $this->_createServer('elastica_test', 'search_api_elasticsearch_elastica_service', array(array('host' => '127.0.0.1', 'port' => '9200')));
    $this->_index = $this->_createIndex('elastica_test_index', 'node', 'elastica_test');
    $this->assertTrue('elastica_test_index', $this->_index->machine_name);
    $this->_client = new SearchApiElasticsearchElastica($this->_server);
  }

  public function testConstructor() {
    $class = $this->_client;
    $this->assertInstanceOf('SearchApiElasticsearchElastica', $class);
    $this->assertInstanceOf('SearchApiElasticsearchAbstractService', $class);
    $this->assertInstanceOf('SearchApiAbstractService', $class);
  }

  public function testAddIndex() {
    $this->_client->addIndex($this->_index);
    $this->assertTrue('elasticsearch_index_drupal_elastica_test_index', $this->_client->getElasticaIndex($this->_index)->getName(), 'Expected "elasticsearch_index_drupal_elastica_test_index". Found ' . $this->_client->getElasticaIndex($this->_index)->getName());
  }

}
