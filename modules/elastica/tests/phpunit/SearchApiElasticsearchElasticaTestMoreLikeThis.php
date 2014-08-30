<?php

/**
 * @file
 * Contains tests for More Like This searches.
 */

class SearchApiElasticsearchElasticaTestMoreLikeThis extends SearchApiElasticsearchElasticaTest {

  /**
   * setUp
   *
   * @access public
   * @return void
   */
  public function setUp() {
    parent::setup();
    $this->_items[] = array(
      'nid' => array(
        'value' => 5,
      ),
      'title' => array(
        'value' =>'bruce wayne batman',
      ),
    );
    $mlt = array(
      'id' => 1,
      'fields' => $this->_index->options['fields'],
    );
    $this->_query->setOption('mlt', $mlt);
  }

  public function testMoreLikeThis() {
    $result_set = $this->_client->search($this->_query);
    $this->assertEquals(1, $result_set['result count']);
  }

}
