<?php

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
  public function __construct(SearchApiIndex $index, SearchApiElasticsearchService $service) {
    $this->search_api_index = $index;
    $this->search_api_service = $service;
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
   * Create Elasticsearch index.
   */
  public function create() {
    $this->setRealIndexName($this->search_api_index);
    $index = $this->getRealIndex($this->search_api_index);
    $index->create($this->getIndexOptions($this->search_api_index), TRUE);
    $index->addAlias($this->search_api_index->machine_name);
  }

  /**
   * Map Elasticsearch alias to Search API index machine name.
   * @param null $alias
   */
  public function addAlias($alias = null) {
    $new_alias = ($alias) ?: $this->search_api_index->machine_name;
    $index = $this->getRealIndex($this->search_api_index);
    $index->addAlias($new_alias);
  }

  /**
   * Delete Elasticsearch index.
   */
  public function delete() {
    $index = $this->getRealIndex($this->search_api_index);
    $index->delete();
  }

  /**
   * Add documents to Elasticsearch index.
   *
   * @param $documents
   * @return array
   */
  public function indexItems($documents) {
    $index = $this->getRealIndex($this->search_api_index);
    $index->addDocuments($documents);
    $index->refresh();
    return array_keys($documents);
  }

  /**
   * Delete all documents in an Elasticsearch index.
   */
  public function deleteAllItems() {
    $match_all = new \Elastica\Query\MatchAll();
    $index = $this->getRealIndex($this->search_api_index);
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
      $documents[] = new \Elastica\Document($id);
    }
    $index = $this->getRealIndex($this->search_api_index);
    $index->deleteDocuments($documents);
  }

  /**
   * Get Elasticsearch index using Search API index machine name as alias.
   * @return \Elastica\Index
   */
  public function getElasticsearchIndex() {
    return $this->search_api_service->getClient()->getIndex($this->search_api_index->machine_name);
  }

  /**
   * Helper method for replacing Elasticsearch index after making changes that
   * cannot be applied via update.
   *
   * @TODO
   *
   * @param SearchApiIndex $index
   * @param \Elastica\Index $old_index
   */
  protected function replaceIndex(SearchApiIndex $index, \Elastica\Index $old_index) {
    $this->search_api_service->addIndex($index);
  }

  /**
   * Create Elasticsearch mapping.
   *
   * @param \Elastica\Type $type
   * @return \Elastica\Type\Mapping
   */
  protected function createElasticsearchMapping(\Elastica\Type $type) {
    $this->setMapping($this->search_api_index);
    $mapping = new \Elastica\Type\Mapping();
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
        'type' => 'integer',
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
   * @param SearchApiIndex $index
   * @return \Elastica\Index
   */
  protected function getRealIndex(SearchApiIndex $index) {
    return $this->search_api_service->getClient()->getIndex($this->getRealIndexName($index));
  }

  /**
   * Get Elasticsearch Index name.
   *
   * @param SearchApiIndex|NULL $index
   * @return null
   */
  protected function getRealIndexName(SearchApiIndex $index = null) {
    return variable_get('search_api_elasticsearch_' . $index->machine_name . '_real_index_name');
  }

  /**
   * Map Elasticsearch alias to index for Drupal.
   *
   * @param SearchApiIndex $index
   */
  protected function setRealIndexName(SearchApiIndex $index) {
    $real_index_name = $index->machine_name . '_' . REQUEST_TIME;
    variable_set('search_api_elasticsearch_' . $index->machine_name . '_real_index_name', $real_index_name);
  }

  /**
   * Delete Elasticsearch Index name from variables.
   *
   * @param SearchApiIndex $index
   */
  protected function deleteRealIndexName(SearchApiIndex $index) {
    variable_del('search_api_elasticsearch_' . $index->machine_name . '_real_index_name');
  }

  /**
   * Get Elasticsearch index configuration options.
   *
   * @param SearchApiIndex $index
   * @return array
   */
  protected function getIndexOptions(SearchApiIndex $index) {
    return (variable_get('search_api_elasticsearch_' . $index->machine_name . '_options')) ?: [];
  }

}