<?php

use Elastica\Document;
use Elastica\Index;
use Elastica\Query\MatchAll;
use Elastica\Type;
use Elastica\Type\Mapping;

class SearchApiElasticsearchIndex {

  /**
   * @var SearchApiIndex
   */
  protected $search_api_index;

  /**
   * @var SearchApiElasticsearchService
   */
  protected $service;

  /**
   * @var array
   */
  protected $mapping;

  /**
   * SearchApiElasticsearchIndex constructor.
   * @param SearchApiIndex $index
   * @param SearchApiElasticsearchService $service
   */
  public function __construct(SearchApiIndex $index, SearchApiElasticsearchService $service = null) {
    $this->search_api_index = $index;
    // We load the service directly for DX, but can fallback to the Search API
    // Server object because it has a proxy to the service.
    $this->search_api_service = isset($service) ? $service : $index->server();
  }

  /**
   * Update fields on an Elasticsearch Index.
   *
   * @return bool
   */
  public function updateFields() {
    $index = $this->getElasticsearchIndex();
    $type = $index->getType($this->search_api_index->machine_name);
    $mapping = $this->createElasticsearchMapping($type);

    try {
      // First we attempt to update the mapping for scenarios where updating is
      // allowed.
      try {
        $mapping->send();
      } catch (Exception $e) {
        // If a mapping cannot be updated, then we create a new index and
        // migrate data to it.
        $this->replaceIndex($this->search_api_index, $index);
      }
    } catch (Exception $e) {
      return FALSE;
    }
    // We still return FALSE here because we do not change update the index
    // alias until data has been moved to new index.
    return FALSE;
  }

  /**
   * Create Elasticsearch a new index.
   */
  public function create() {
    $index = $this->getRealIndex();
    $index->create($this->search_api_index->options['search_api_elasticsearch'] ?: [], TRUE);
    $index->addAlias($this->search_api_index->machine_name);
  }

  /**
   * Add an Elasticsearch index, but don't expose it to Drupal via alias.
   */
  public function add() {
    $name = $this->getNewIndexName();
    $index = $this->getRealIndex($name);
    $index->create($this->search_api_index->options['search_api_elasticsearch'] ?: [], TRUE);
  }

  /**
   * Map Elasticsearch alias to Search API index machine name.
   * @param null $alias
   */
  public function addAlias($alias = null) {
    $new_alias = ($alias) ?: $this->search_api_index->machine_name;
    $index = $this->getRealIndex();
    $index->addAlias($new_alias);
  }

  /**
   * Delete Elasticsearch index.
   */
  public function delete() {
    $index = $this->getRealIndex();
    $index->delete();
  }

  /**
   * Add documents to Elasticsearch index.
   *
   * @param $documents
   * @return array
   */
  public function indexItems($documents) {
    //watchdog('search_api_elasticsearch', print_r($documents, TRUE));
    $index = $this->getRealIndex();
    $index->addDocuments($documents);
    $index->refresh();
    return array_keys($documents);
  }

  /**
   * Delete all documents in an Elasticsearch index.
   */
  public function deleteAllItems() {
    $match_all = new MatchAll();
    $index = $this->getRealIndex();
    $index->deleteByQuery($match_all);
  }

  /**
   * Delete documents from Elasticsearch.
   *
   * @param $ids
   */
  public function deleteItems($ids) {
    $documents = [];
    foreach ($ids as $id) {
      $documents[] = new Document($id);
    }
    $index = $this->getRealIndex();
    $index->deleteDocuments($documents);
  }

  /**
   * Get Elasticsearch index using Search API index machine name as alias.
   * @return Index
   */
  public function getElasticsearchIndex() {
    return $this->search_api_service->getClient()->getIndex($this->search_api_index->machine_name);
  }

  /**
   * Helper method for replacing Elasticsearch index after making changes that
   * cannot be applied via update.
   *
   * @TODO
   * Consider using aliases to create a new Elasticsearch Index and
   * migrate indexed items to new Elasticsearch Index on the Elasticsearch
   * side before deleting the old index. This will allow for zero-downtime
   * updates.
   *
   * @param SearchApiIndex $index
   * @param Index $old_index
   */
  protected function replaceIndex(SearchApiIndex $index, Index $old_index) {
    $this->delete();
    $this->search_api_service->addIndex($index);
  }

  /**
   * Create Elasticsearch mapping.
   *
   * @param Type $type
   * @return Mapping
   */
  protected function createElasticsearchMapping(Type $type) {
    $this->setMapping($this->search_api_index);
    $mapping = new Mapping();
    $mapping->setType($type);
    $mapping->setParam('_all', ['enabled' => FALSE]);
    $mapping->setProperties($this->mapping);
    return $mapping;
  }

  /**
   * Set Elasticsearch mapping.
   *
   * @param SearchApiIndex $index
   */
  protected function setMapping(SearchApiIndex $index) {
    $this->mapping = [
      'id' => [
        'type' => 'string',
        'include_in_all' => FALSE,
      ],
    ];
    foreach ($index->getFields() as $id => $field) {
      $this->mapping[$id] = $this->getFieldMapping($field);
    }
  }

  /**
   * Map Search API field types to Elasticsearch field types.
   *
   * @param $field
   * @return array|null
   */
  protected function getFieldMapping($field) {
    $field_type = (isset($field['real_type'])) ? $field['real_type'] : $field['type'];
    $type = search_api_extract_inner_type($field_type);
    switch ($type) {
      case 'text':
        return array(
          'type' => 'string',
          'boost' => $field['boost'],
        );
      case 'uri':
      case 'string':
      case 'token':
        return array(
          'type' => 'string',
          'index' => 'not_analyzed',
        );
      case 'integer':
      case 'duration':
        return array(
          'type' => 'integer',
        );
      case 'boolean':
        return array(
          'type' => 'boolean',
        );
      case 'decimal':
        return array(
          'type' => 'float',
        );
      case 'date':
        return array(
          'type' => 'date',
          'format' => 'date_time',
        );
      case 'location':
        return array(
          'type' => 'geo_point',
          'lat_lon' => TRUE,
        );
      default:
        return NULL;
    }
  }

  /**
   * Get Elasticsearch index.
   *
   * @param string $name
   *
   * @return Index
   */
  protected function getRealIndex($name = null) {
    $index_name = isset($name) ? $name : $this->getCurrentIndexName();
    return $this->search_api_service->getClient()->getIndex($index_name);
  }

  /**
   * Get a new Elasticsearch index name.
   *
   * @return string;
   */
  protected function getNewIndexName() {
    return $this->search_api_index->machine_name . '_' . REQUEST_TIME;
  }

  /**
   * Get current Elasticsearch Index name.
   *
   * @return string
   */
  protected function getCurrentIndexName() {
    return $this->search_api_index->options['search_api_elasticsearch']['current_index'];
  }

  /**
   * Map Elasticsearch alias to index for Drupal.
   * @param SearchApiIndex $index
   */
  static function setCurrentIndexName(SearchApiIndex $index) {
    $index->options['search_api_elasticsearch']['current_index'] = $index->machine_name . '_' . REQUEST_TIME;
  }

}