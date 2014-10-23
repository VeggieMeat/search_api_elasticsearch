<?php

/**
 * @file
 * Contains searcher for Elastica client.
 */

class SearchApiElasticsearchElasticaSearcher {

  public function __construct(SearchApiQueryInterface $search_api_query) {
    $this->search_api_query = $search_api_query;
  }

  public function build() {
    // Query options.
    $query_options = $this->getSearchQueryOptions($this->search_api_query);

    // Main query.
    $elastica_query = new Elastica\Query();
    $elastica_query->setFrom($query_options['query_offset']);
    $elastica_query->setSize($query_options['query_limit']);

    // Search API Location support.
    if (!empty($query_options['spatials'])) {
      foreach ($query_options['spatials'] as $i => $spatial) {
        if (empty($spatial['field']) || empty($spatial['lat']) || empty($spatial['lon'])) {
          continue;
        }

        $field = $spatial['field'];
        $point = array(
          'lat' => (float) $spatial['lat'],
          'lon' => (float) $spatial['lon'],
        );

        $radius = isset($spatial['radius']) ? (float) $spatial['radius'] : NULL;

        $query_options['query_search_filter'] = new Elastica\Filter\GeoDistance($field, $point, $radius);
      }
    }

    // Build the query.
    if (!empty($query_options['query_search_string']) && !empty($query_options['query_search_filter'])) {
      $elastica_query->setQuery(new Elastica\Query\Filtered($query_options['query_search_string'], $query_options['query_search_filter']));
    }
    elseif (!empty($query_options['query_search_string'])) {
      $elastica_query->setQuery($query_options['query_search_string']);
    }
    elseif (!empty($query_options['query_search_filter'])) {
      $elastica_query->setFilter($query_options['query_search_filter']);
    }

    // Sort.
    if (!empty($query_options['sort'])) {
      $elastica_query->setSort($query_options['sort']);
    }

    $this->query = $elastica_query;
    return $this->query;
  }

  public function getOptions() {
    $query_options = $this->search_api_query->getOptions();
    $this->mapOptions($query_options);

    return $query_options;
  }

  public function search($type, $elastica_query, $options) {
    if (!empty($options['mlt'])) {
      SearchApiElasticsearchElasticaMLT::build($type, $options, $this->query);
    }

    drupal_alter('search_api_elasticsearch_elastica_query', $this->query, $this->search_api_query);

    return $type->search($query);
  }

  protected function mapOptions(&$options) {
    // Key: Search API key.
    // Value: Our key.
    $options_map = array(
      'query_offset' => 'offset',
      'query_limit' => 'limit',
      'search_api_location' => 'spatials',
      'search_api_mlt' => 'mlt',
    );

    foreach ($options as $key => $value) {
      if (in_array($key, array_keys($options_map))) {
        $options[$options_map[$key]] = $value;
      }
    }
  }

}
