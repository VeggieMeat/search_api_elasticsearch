<?php

/**
 * @file
 * Contains tests for More Like This searches.
 */

class SearchApiElasticsearchElasticaMoreLikeThisTest extends SearchApiElasticsearchElasticaBaseTest {

  /**
   * setUp
   *
   * @access public
   * @return void
   */
  public function setUp() {
    parent::setUp();
    $this->_index = $this->createIndex('elastica_test_mlt_index', 'node', 'elastica_test_mlt');
    $this->_index->options['fields'] = array(
      'nid' => array(
        'type' => 'integer',
      ),
      'title' => array(
        'type' => 'text',
      ),
    );
    $this->_items = array(
      '1' => array(
        'nid' => array(
          'value' => 1,
        ),
        'title' => array(
          'value' => 'bruce wayne batman',
        ),
      ),
      '2' => array(
        'nid' => array(
          'value' => 2,
        ),
        'title' => array(
          'value' => 'bruce wayne',
        ),
      ),
      '3' => array(
        'nid' => array(
          'value' => 3,
        ),
        'title' => array(
          'value' => 'batman',
        ),
      ),
      '4' => array(
        'nid' => array(
          'value' => 4,
        ),
        'title' => array(
          'value' => 'superman',
        ),
      ),
    );
    $this->_client->indexItems($this->_index, $this->_items);
    $this->_client->getElasticaIndex($this->_index)->refresh();
    $mlt = array(
      'id' => 1,
      'fields' => array('title'),
    );
    $this->_query = new SearchApiQuery($this->_index);
    $this->_query->setOption('search_api_mlt', $mlt);
  }

  /**
   * testMoreLikeThis
   *
   * @access public
   * @return void
   */
  public function testMoreLikeThis() {
    $result_set = $this->_client->search($this->_query);
    $this->assertEquals(2, $result_set['result count']);
  }

}
