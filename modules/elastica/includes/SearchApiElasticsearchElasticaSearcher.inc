<?php

/**
 * @file
 * Contains searcher for Elastica client.
 */

class SearchApiElasticsearchElasticaSearcher {

  public function __construct(SearchApiQueryInterface $search_api_query) {
    $this->search_api_query = $search_api_query;
  }

  public function getOptions() {
    $query_options = $this->search_api_query->getOptions();
    $this->mapOptions($query_options);

    return $query_options;
  }

  public function search($type, $query, $options) {
    if (!empty($options['mlt'])) {
      SearchApiElasticsearchElasticaMLT::build($type, $options, $query);
    }

    drupal_alter('search_api_elasticsearch_elastica_query', $query, $this->search_api_query);

    return $type->search($query);
  }

  protected function mapOptions(&$options) {
    // Key: Our key.
    // Value: Search API key.
    $options_map = array(
      'offset' => 'query_offset',
      'limit' => 'query_limit',
      'spatials' => 'search_api_location',
      'mlt' => 'search_api_mlt',
    );

    foreach ($options as $key => $value) {
      if (in_array($key, $options_map)) {
        $options[$options_map[$key]] = $options[$value];
      }
    }
  }

}
