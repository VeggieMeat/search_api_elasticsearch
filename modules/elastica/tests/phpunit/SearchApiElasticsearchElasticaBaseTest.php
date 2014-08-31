<?php

abstract class SearchApiElasticsearchElasticaBaseTest extends SearchApiElasticsearchBaseTest {

  /**
   * setUp
   *
   * @access public
   * @return void
   */
  public function setUp() {
    $this->_server = $this->createServer('elastica_test', 'search_api_elasticsearch_elastica_service', array(array('host' => '127.0.0.1', 'port' => '9200')));
    $this->_client = new SearchApiElasticsearchElastica($this->_server);
  }
}
