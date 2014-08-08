<?php

/**
 * @file
 * Contains tests for SearchApiElasticsearchElastica.
 */

class SearchApiElasticsearchElasticaTest extends SearchApiElasticsearchBaseTest {

  /**
   * setUp
   *
   * @access public
   * @return void
   */
  public function setUp() {
    $this->_server = $this->createServer('elastica_test', 'search_api_elasticsearch_elastica_service', array(array('host' => '127.0.0.1', 'port' => '9200')));
    $this->_index = $this->createIndex('elastica_test_index', 'node', 'elastica_test');
    $this->_client = new SearchApiElasticsearchElastica($this->_server);
  }

  /**
   * testConstructor
   *
   * @access public
   * @return void
   */
  public function testConstructor() {
    $class = $this->_client;
    $this->assertInstanceOf('SearchApiElasticsearchElastica', $class);
    $this->assertInstanceOf('SearchApiElasticsearchAbstractService', $class);
    $this->assertInstanceOf('SearchApiAbstractService', $class);
  }

  /**
   * testAddIndex
   *
   * @access public
   * @return void
   */
  public function testAddIndex() {
    $this->_client->addIndex($this->_index);
    $this->assertSame('elasticsearch_index_drupal_elastica_test_index', $this->_client->getElasticaIndex($this->_index)->getName(), 'Expected "elasticsearch_index_drupal_elastica_test_index". Found ' . $this->_client->getElasticaIndex($this->_index)->getName());
  }

  /**
   * testSupportsFeature
   *
   * @access public
   * @return void
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
   * testRemoveIndex
   *
   * @access public
   * @return void
   */
  public function testRemoveIndex() {
    $status = new \Elastica\Status($this->_client->getElasticaClient());
    $this->assertTrue($status->indexExists($this->_client->getElasticaIndex($this->_index)->getName()));
    $this->_client->removeIndex($this->_index);
    $status->refresh();
    $this->assertFalse($status->indexExists($this->_client->getElasticaIndex($this->_index)->getName()));
  }

  /**
   * testFieldsUpdated
   *
   * @access public
   * @return void
   */
  public function testFieldsUpdated() {
    $this->markTestIncomplete(
      'This test has not yet been implemented.'
    );
    $this->assertTrue($this->_client->fieldsUpdated($this->_index));
  }

  /**
   * testPostUpdate
   *
   * @access public
   * @return void
   */
  public function testPostUpdate() {
    $this->assertFalse($this->_client->postUpdate());
  }

  /**
   * testIndexItems
   *
   * @access public
   * @return void
   */
  public function testIndexItems() {
    $items = array(
      '1' => array(
        'nid' => array(
          'value' => 1,
        ),
        'title' => array(
          'value' => 'batman',
        ),
      ),
      '2' => array(
        'nid' => array(
          'value' => 2,
        ),
        'title' => array(
          'value' => 'robin',
        ),
      ),
      '3' => array(
        'nid' => array(
          'value' => 3,
        ),
        'title' => array(
          'value' => 'catwoman',
        ),
      ),
      '4' => array(
        'nid' => array(
          'value' => 4,
        ),
        'title' => array(
          'value' => 'joker',
        ),
        'friends' => array(
          'value' => array(
            'harleyquinn',
            'penguin',
          ),
        ),
        'rivals' => array(
          'value' => array(
            'batman' => array(
              'value' => 'Bruce Wayne',
            ),
          ),
        ),
      ),
    );
    $this->_client->indexItems($this->_index, $items);
    $this->_client->getElasticaIndex($this->_index)->refresh();
    $result_set = $this->_client->getElasticaType($this->_index)->search('batman');
    $this->assertEquals(1, $result_set->count());
    $count = $this->_client->getElasticaType($this->_index)->count('batman');
    $this->assertEquals(1, $count);
    $result = $result_set->current();
    $this->assertNotFalse($result);
    $this->assertEquals(1, $result->getId());
    $data = $result->getData();
    $this->assertEquals('batman', $data['title']);

    $this->assertEmpty($this->_client->indexItems($this->_index, array()));
  }

  /**
   * testDeleteItem
   *
   * @access public
   * @return void
   */
  public function testDeleteItem() {
    $items = array(
      '1' => array(
        'nid' => array(
          'value' => 1,
        ),
        'title' => array(
          'value' => 'batman',
        ),
      ),
      '2' => array(
        'nid' => array(
          'value' => 2,
        ),
        'title' => array(
          'value' => 'robin',
        ),
      ),
      '3' => array(
        'nid' => array(
          'value' => 3,
        ),
        'title' => array(
          'value' => 'catwoman',
        ),
      ),
    );
    $this->_client->indexItems($this->_index, $items);
    $this->_client->getElasticaIndex($this->_index)->refresh();
    $result_set = $this->_client->getElasticaType($this->_index)->search('batman');
    $this->assertEquals(1, $result_set->count());
    $result = $result_set->current();
    $this->assertNotFalse($result);
    $this->assertEquals(1, $result->getId());
    $this->_client->deleteItems(array('1'), $this->_index);
    $this->_client->getElasticaIndex($this->_index)->refresh();
    $result_set = $this->_client->getElasticaType($this->_index)->search('batman');
    $this->assertEquals(0, $result_set->count());
    $result_set = $this->_client->getElasticaType($this->_index)->search('robin');
    $this->assertEquals(1, $result_set->count());
  }

  /**
   * testDeleteMultipleItems
   *
   * @access public
   * @return void
   */
  public function testDeleteMultipleItems() {
    $items = array(
      '1' => array(
        'nid' => array(
          'value' => 1,
        ),
        'title' => array(
          'value' => 'batman',
        ),
      ),
      '2' => array(
        'nid' => array(
          'value' => 2,
        ),
        'title' => array(
          'value' => 'robin',
        ),
      ),
      '3' => array(
        'nid' => array(
          'value' => 3,
        ),
        'title' => array(
          'value' => 'catwoman',
        ),
      ),
    );
    $this->_client->indexItems($this->_index, $items);
    $this->_client->getElasticaIndex($this->_index)->refresh();
    $result_set = $this->_client->getElasticaType($this->_index)->search('batman');
    $this->assertEquals(1, $result_set->count());
    $result = $result_set->current();
    $this->assertNotFalse($result);
    $this->assertEquals(1, $result->getId());
    $result_set = $this->_client->getElasticaType($this->_index)->search('robin');
    $this->assertEquals(1, $result_set->count());
    $result = $result_set->current();
    $this->assertNotFalse($result);
    $this->assertEquals(2, $result->getId());
    $this->_client->deleteItems(array('1', '2'), $this->_index);
    $this->_client->getElasticaIndex($this->_index)->refresh();
    $result_set = $this->_client->getElasticaType($this->_index)->search('batman');
    $this->assertEquals(0, $result_set->count());
    $result_set = $this->_client->getElasticaType($this->_index)->search('robin');
    $this->assertEquals(0, $result_set->count());
   $result_set = $this->_client->getElasticaType($this->_index)->search('catwoman');
    $this->assertEquals(1, $result_set->count());
  }

  /**
   * testDeleteAllItems
   *
   * @access public
   * @return void
   */
  public function testDeleteAllItems() {
    $items = array(
      '1' => array(
        'nid' => array(
          'value' => 1,
        ),
        'title' => array(
          'value' => 'batman',
        ),
      ),
      '2' => array(
        'nid' => array(
          'value' => 2,
        ),
        'title' => array(
          'value' => 'robin',
        ),
      ),
      '3' => array(
        'nid' => array(
          'value' => 3,
        ),
        'title' => array(
          'value' => 'catwoman',
        ),
      ),
    );
    $this->_client->indexItems($this->_index, $items);
    $this->_client->getElasticaIndex($this->_index)->refresh();
    $result_set = $this->_client->getElasticaType($this->_index)->search('batman');
    $this->assertEquals(1, $result_set->count());
    $result = $result_set->current();
    $this->assertNotFalse($result);
    $this->assertEquals(1, $result->getId());
    $result_set = $this->_client->getElasticaType($this->_index)->search('robin');
    $this->assertEquals(1, $result_set->count());
    $result = $result_set->current();
    $this->assertNotFalse($result);
    $this->assertEquals(2, $result->getId());
    $result_set = $this->_client->getElasticaType($this->_index)->search('catwoman');
    $this->assertEquals(1, $result_set->count());
    $result = $result_set->current();
    $this->assertNotFalse($result);
    $this->assertEquals(3, $result->getId());
    $this->_client->deleteItems('all', $this->_index);
    $this->_client->getElasticaIndex($this->_index)->refresh();
    $result_set = $this->_client->getElasticaType($this->_index)->search('batman');
    $this->assertEquals(0, $result_set->count());
    $result_set = $this->_client->getElasticaType($this->_index)->search('robin');
    $this->assertEquals(0, $result_set->count());
    $result_set = $this->_client->getElasticaType($this->_index)->search('catwoman');
    $this->assertEquals(0, $result_set->count());
  }

  /**
   * testDeleteAllItemsFromAllIndexes
   *
   * @access public
   * @return void
   */
  public function testDeleteAllItemsFromAllIndexes() {
    $items = array(
      '1' => array(
        'nid' => array(
          'value' => 1,
        ),
        'title' => array(
          'value' => 'batman',
        ),
      ),
      '2' => array(
        'nid' => array(
          'value' => 2,
        ),
        'title' => array(
          'value' => 'robin',
        ),
      ),
      '3' => array(
        'nid' => array(
          'value' => 3,
        ),
        'title' => array(
          'value' => 'catwoman',
        ),
      ),
    );
    $this->_client->indexItems($this->_index, $items);
    $this->_client->getElasticaIndex($this->_index)->refresh();
    $result_set = $this->_client->getElasticaType($this->_index)->search('batman');
    $this->assertEquals(1, $result_set->count());
    $result = $result_set->current();
    $this->assertNotFalse($result);
    $this->assertEquals(1, $result->getId());
    $result_set = $this->_client->getElasticaType($this->_index)->search('robin');
    $this->assertEquals(1, $result_set->count());
    $result = $result_set->current();
    $this->assertNotFalse($result);
    $this->assertEquals(2, $result->getId());
    $result_set = $this->_client->getElasticaType($this->_index)->search('catwoman');
    $this->assertEquals(1, $result_set->count());
    $result = $result_set->current();
    $this->assertNotFalse($result);
    $this->assertEquals(3, $result->getId());
    $this->_client->deleteItems('all');
    $this->_client->getElasticaIndex($this->_index)->refresh();
    $this->markTestIncomplete(
      'This test is currently incomplete.'
    );
    $result_set = $this->_client->getElasticaType($this->_index)->search('batman');
    $this->assertEquals(0, $result_set->count());
    $result_set = $this->_client->getElasticaType($this->_index)->search('robin');
    $this->assertEquals(0, $result_set->count());
    $result_set = $this->_client->getElasticaType($this->_index)->search('catwoman');
    $this->assertEquals(0, $result_set->count());
  }

  /**
   * testGetSettings
   *
   * @access public
   * @return void
   */
  public function testGetSettings() {
    $this->assertNotFalse($this->_client->getSettings($this->_index));
  }

  /**
   * testViewSettings
   *
   * @access public
   * @return void
   */
  public function testViewSettings() {
    $this->assertNotFalse($this->_client->viewSettings());
  }
}
