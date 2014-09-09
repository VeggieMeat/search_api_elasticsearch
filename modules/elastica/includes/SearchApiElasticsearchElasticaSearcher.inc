<?php

/**
 * @file
 * Contains searcher for Elastica client.
 */

class SearchApiElasticsearchElasticaSearcher {

  public function __construct(SearchApiQueryInterface $query) {
    $this->query = $query;
    $this->options = $this->getSearchQueryOptions($query);
  }

  protected function addFacets() {
    // SEARCH API FACETS.
    $facets = $this->query->getOption('search_api_facets');
    $index_fields = $this->getIndexFields($this->query);

    if (!empty($facets)) {
      // Loop trough facets.
      foreach ($facets as $facet_id => $facet_info) {
        $facet = NULL;
        $field_id = $facet_info['field'];

        // Skip if not recognized as a known field.
        if (!isset($index_fields[$field_id])) {
          continue;
        }

        $facet_missing = $facet_info['missing'];

        $field_type = search_api_extract_inner_type($index_fields[$field_id]['type']);

        // TODO: handle different types (GeoDistance and so on).
        if ($field_type === 'date') {
          $facet = $this->createDateFieldFacet($facet_id);
        }
        else {
          $facet = new Elastica\Facet\Terms($facet_id);
          // We may want missing facets.
          $facet->setAllTerms($facet_missing);
        }

        // Add the facet.
        if (!empty($facet)) {
          // Add facet options.
          $this->addFacetOptions($facet, $facet_info);
          $this->elastica_query->addFacet($facet);
        }
      }
    }
  }

  /**
   * Helper function that add options and return facet.
   */
  protected function addFacetOptions(&$facet, $facet_info) {

    $facet_limit = $this->getFacetLimit($facet_info);
    $facet_search_filter = $this->getFacetSearchFilter($this->query, $facet_info);
    // Set the field.
    $facet->setField($facet_info['field']);

    // OR facet. We remove filters affecting the assiociated field.
    // TODO: distinguish between normal filters and facet filters.
    // See http://drupal.org/node/1390598.

    // Filter the facet.
    if (!empty($facet_search_filter)) {
      $facet->setFilter($facet_search_filter);
    }

    // Limit the number of returned entries.
    if ($facet_limit > 0 && method_exists($facet, 'setSize')) {
      $facet->setSize($facet_limit);
    }
  }

  /**
   * Helper function create Facet for date field type.
   */
  protected function createDateFieldFacet($facet_id) {

    $date_interval = $this->getDateFacetInterval($facet_id);

    $facet = new Elastica\Facet\DateHistogram($facet_id);

    $facet->setInterval($date_interval);

    // Maybe get php timezone?
    $facet->setTimezone('UTC');

    // Use factor 1000 as we store dates as seconds from epoch
    // not milliseconds.
    $facet->setParam('factor', 1000);

    return $facet;
  }
  protected function build() {
    $this->elastica_query = new Elastica\Query();
    $this->elastica_query->setFrom($this->options['query_offset']);
    $this->elastica_query->setSize($this->options['query_limit']);
    $this->buildSpatials($this->options);
    $this->addFacets();
    // Build the query.
    if (!empty($this->options['query_search_string']) && !empty($this->options['query_search_filter'])) {
      $this->elastica_query->setQuery(new Elastica\Query\Filtered($this->options['query_search_string'], $query_options['query_search_filter']));
    }
    elseif (!empty($this->options['query_search_string'])) {
      $this->elastica_query->setQuery($this->options['query_search_string']);
    }
    elseif (!empty($this->options['query_search_filter'])) {
      $this->elastica_query->setFilter($this->options['query_search_filter']);
    }

    // Sort.
    if (!empty($this->options['sort'])) {
      $this->elastica_query->setSort($this->options['sort']);
    }

    drupal_alter('search_api_elasticsearch_elastica_query', $this->elastica_query, $this->query);
  }

  protected function buildSpatials() {
    if (!empty($this->options['spatials'])) {
      foreach ($this->options['spatials'] as $i => $spatial) {
        if (empty($spatial['field']) || empty($spatial['lat']) || empty($spatial['lon'])) {
          continue;
        }
        $field = $spatial['field'];
        $point = array(
          'lat' => (float) $spatial['lat'],
          'lon' => (float) $spatial['lon'],
        );

        $radius = isset($spatial['radius']) ? (float) $spatial['radius'] : NULL;

        $this->options['query_search_filter'] = new Elastica\Filter\GeoDistance($field, $point, $radius);
      }
    }
  }

  public function search($type) {
    $this->build();
    if (!empty($this->options['mlt'])) {
      return SearchApiElasticsearchElasticaMLT::search($type, $this->options, $this->elastica_query);
    }
    // Show Elasticsearch query string from Elastica
    // as json output when views debug output is enabled.
    if (function_exists('vpr') &&
      $elastica_param_query = $elastica_query->getParam('query')) {
      vpr(drupal_json_encode($elastica_param_query));
    }
    return $type->search($this->elastica_query);
  }

}
