<?php

  /**
   * @file
   * Contains tests for SearchApiElasticsearchElastica.
   */

class SearchApiElasticsearchElasticaTest extends SearchApiElasticsearchBaseTest {

  public function setUp() {
    $this->_server = $this->_createServer('elastica_test', 'search_api_elasticsearch_elastica_service', array(array('host' => '127.0.0.1', 'port' => '9200')));
    $this->_index = $this->_createIndex('elastica_test_index', 'node', 'elastica_test');
    $this->_client = new SearchApiElasticsearchElastica($this->_server);
  }

  public function testConstructor() {
    $class = $this->_client;
    $this->assertInstanceOf('SearchApiElasticsearchElastica', $class);
    $this->assertInstanceOf('SearchApiElasticsearchAbstractService', $class);
    $this->assertInstanceOf('SearchApiAbstractService', $class);
  }

  /**
   * @covers SearchApiElasticsearchElastica::addIndex
   */
  public function testAddIndex() {
    $this->_client->addIndex($this->_index);
    $this->assertSame('elasticsearch_index_drupal_elastica_test_index', $this->_client->getElasticaIndex($this->_index)->getName(), 'Expected "elasticsearch_index_drupal_elastica_test_index". Found ' . $this->_client->getElasticaIndex($this->_index)->getName());
  }

  /**
   * @covers SearchApiElasticsearchElastica::supportsFeature
   */
  public function testSupportsFeature() {
    $features = array(
      'search_api_autocomplete',
      'search_api_data_type_location',
      'search_api_facets',
      'search_api_facets_operator_or',
      'search_api_mlt',
      'search_api_service_extra',
      'search_api_test_fail',
    );
    foreach ($features as $feature) {
      if ($feature === 'search_api_test_fail') {
        $this->assertFalse($this->_client->supportsFeature($feature));
      }
      else {
        $this->assertTrue($this->_client->supportsFeature($feature));
      }
    }
  }

  /**
   * @covers SearchApiElasticsearchElastica::removeIndex
   */
  public function testRemoveIndex() {
    $response = $this->_client->removeIndex($this->_index);
    $this->assertTrue($reponse->isOK());
  }

  /**
   * @covers SearchApiElasticsearchElastica::fieldsUpdated
   */
  public function testFieldsUpdated() {
    $this->assertTrue($this->_client->fieldsUpdated($this->_index));
  }

  /**
   * @covers SearchApiElasticsearchElastica::postUpdate
   */
  public function testPostUpdate() {
    $this->assertFalse($this->_client->postUpdate());
  }

}
