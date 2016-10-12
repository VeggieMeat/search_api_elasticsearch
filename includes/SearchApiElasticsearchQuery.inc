<?php
use Elastica\Aggregation\AbstractSimpleAggregation;
use Elastica\Aggregation\DateRange;
use Elastica\Aggregation\Filter;
use Elastica\Aggregation\Range;
use Elastica\Aggregation\Terms;
use Elastica\Query;
use Elastica\Query\BoolQuery;
use Elastica\Query\Exists;
use Elastica\Query\QueryString;
use Elastica\Query\Range as RangeQuery;
use Elastica\ResultSet;

/**
 * @file
 */

class SearchApiElasticsearchQuery {

  /**
   * @var SearchApiQueryInterface
   */
  protected $search_api_query;

  /**
   * @var SearchApiElasticsearchService
   */
  protected $search_api_service;

  /**
   * @var Query
   */
  protected $query;

  /**
   * @var array
   */
  protected $query_options;

  /**
   * SearchApiElasticsearchQuery constructor.
   * @param SearchApiQueryInterface $query
   * @param SearchApiElasticsearchService $service
   */
  public function __construct(SearchApiQueryInterface $query, SearchApiElasticsearchService $service) {
    $this->search_api_query = $query;
    $this->search_api_service = $service;
    $this->index_fields = $query->getIndex()->getFields();
    $this->query_options = $query->getOptions();
  }

  /**
   * Build Elastica query for Elasticsearch.
   *
   * @throws Exception
   */
  protected function build() {
    $this->query = new Query();
    $offset = (empty($this->query_options['offset'])) ? 0 : $this->query_options['offset'];
    $limit = (empty($this->query_options['limit'])) ? 10 : $this->query_options['limit'];

    $keys = $this->search_api_query->getKeys();
    if (!empty($keys)) {
      if (is_string($keys)) {
        $keys = array($keys);
      }

      $full_text_fields = $this->search_api_query->getFields();
      $string = $this->flattenKeys($keys, $this->query_options['parse mode']);

      if (!empty($string)) {
        $query_string = new QueryString($string);
        $query_string->setFields(array_values($full_text_fields));
      }
    }

    $sort = $this->getSortQuery();
    $parsed_filters = $this->parseFilter($this->search_api_query->getFilter());
    if (!empty($parsed_filters)) {
      $search_filter = $parsed_filters[0];
    }

    $this->query->setFrom($offset);
    $this->query->setSize($limit);

    if (isset($query_string) && isset($search_filter)) {
      $bool_query = new BoolQuery();
      $bool_query->addMust($query_string);
      $bool_query->addFilter($search_filter);
      $this->query->setQuery($bool_query);
    }
    elseif (isset($query_string)) {
      $this->query->setQuery($query_string);
    }
    elseif (isset($search_filter)) {
      $this->query->setPostFilter($search_filter);
    }

    if (!empty($sort)) {
      $this->query->setSort($sort);
    }
  }

  /**
   * Execute a search.
   *
   * @return array
   */
  public function search() {
    $this->build();
    $index = new SearchApiElasticsearchIndex($this->search_api_query->getIndex(), $this->search_api_service);
    $elasticsearch_index = $index->getElasticsearchIndex();
    $result_set = $elasticsearch_index->search($this->query);
    return $this->parseResults($result_set);
  }

  /**
   * Parse Elasticsearch results for use by Search API.
   *
   * @param ResultSet $result_set
   * @return array
   */
  protected function parseResults(ResultSet $result_set) {
    $search_result = ['results' => []];
    $search_result['result count'] = $result_set->getTotalHits();

    foreach ($result_set->getResults() as $result) {
      $id = $result->getId();
      $search_result['results'][$id] = [
        'id' => $id,
        'score' => $result->getScore(),
        'fields' => $result->getSource(),
      ];
    }

    $search_result['search_api_facets'] = $this->parseAggregations($result_set);

    return $search_result;
  }

  /**
   * Parse aggregations for Facet API.
   *
   * @param ResultSet $result_set
   * @return array
   */
  protected function parseAggregations(ResultSet $result_set) {
    $result = [];
    $aggregations = $this->search_api_query->getOption('search_api_facets');

    if (!empty($aggregations) && $result_set->hasAggregations()) {
      foreach ($result_set->getAggregations() as $aggregation_id => $aggregation_data) {
        if (isset($aggregations[$aggregation_id])) {
          $aggregation_info = $aggregations[$aggregation_id];
          $aggregation_min_count = $aggregation_info['min_count'];

          $field_id = $aggregation_info['field'];
          $field_type = search_api_extract_inner_type($this->index_fields[$field_id]['type']);

          if ($field_type === 'date') {
            foreach ($aggregation_data['buckets'] as $entry) {
              if ($entry['count'] >= $aggregation_min_count) {
                $result[$aggregation_id][] = [
                  'count' => $entry['count'],
                  'filter' => '"' . ($entry['time'] / 1000) . '"',
                ];
              }
            }
          }
          else {
            foreach ($aggregation_data['buckets'] as $term) {
              if ($term['doc_count'] >= $aggregation_min_count) {
                $result[$aggregation_id][] = array(
                  'count' => $term['doc_count'],
                  'filter' => '"' . $term['key'] . '"',
                );
              }
            }
          }
        }
      }
    }
    return $result;
  }

  /**
   * Add aggregations to an Elasticsearch query.
   */
  protected function addAggregations() {
    $aggregations = $this->search_api_query->getOption('search_api_facets');
    if (!empty($aggregations)) {
      foreach ($aggregations as $aggregation_id => $aggregation_info) {
        $aggregation = null;
        $field_id = $aggregation_info['field'];
        if (!isset($this->index_fields[$field_id])) {
          continue;
        }
        $aggregation_missing = $aggregation_info['missing'];
        $field_type = search_api_extract_inner_type($this->index_fields[$field_id]['type']);

        if ($field_type === 'date') {
          $aggregation = new DateRange($aggregation_id);
          $aggregation->setField($aggregation_id);
          $aggregation->addRange(strtotime('-1 day'), time(), '1 Day');
          $aggregation->addRange(strtotime('-7 day'), time(), '1 Week');
          $aggregation->addRange(strtotime('-14 day'), time(), '2 Weeks');
          $aggregation->addRange(strtotime('-21 day'), time(), '3 Weeks');
          $aggregation->addRange(strtotime('-365 day'), time(), 'All time');
          $aggregation->setFormat("dd MM yyyy");
        }
        elseif ($field_type === 'string') {
          if (strpos($aggregation_id, 'latlon') !== false) {

          }
          else {
            $aggregation = new Terms($aggregation_id);
          }
        }

        if (!empty($aggregation)) {
          $this->setAggregationOptions($aggregation, $field_type, $aggregation_info);
          $this->query->addAggregation($aggregation);
        }
      }
    }
  }

  /**
   * Configure an aggregation.
   * @param AbstractSimpleAggregation $aggregation
   * @param $field_type
   * @param $aggregation_info
   */
  protected function setAggregationOptions(AbstractSimpleAggregation $aggregation, $field_type, $aggregation_info) {
    $aggregation_limit = $this->getAggregationLimit($aggregation_info);
    $aggregation_filter = $this->getAggregationFilter($aggregation_info);
    $aggregation->setField($aggregation_info['field']);

    if (!empty($aggregation_filter)) {
      $filter = new Filter($aggregation_info['field']);
      $range = new Range($aggregation_info['field']);
      $filter->addAggregation($range);
      $filter->setFilter($aggregation_filter);
      $aggregation->addAggregation($filter);
    }

    if ($aggregation_limit > 0 && method_exists($aggregation, 'setSize')) {
      $aggregation->setSize($aggregation_limit);
    }
  }

  /**
   * Helper method to get limits for aggregations.
   * @param $aggregation_info
   * @return int
   */
  protected function getAggregationLimit($aggregation_info) {
    $aggregation_limit = !empty($aggregation_info['limit']) ? $aggregation_info['limit'] : -1;
    if ($aggregation_limit < 0) {
      $aggregation_limit = $this->getOption('facet_limit', 10);
    }
    return $aggregation_limit;
  }

  /**
   * Helper method to get filters for aggregations.
   *
   * @param $aggregation_info
   * @return array|string
   */
  protected function getAggregationFilter($aggregation_info) {
    $aggregation_filter = '';
    if (isset($aggregation_info['operator']) && drupal_strtolower($aggregation_info['operator']) === 'or') {
      $aggregation_filter = $this->parseFilter($this->search_api_query->getFilter(), $aggregation_info['field']);
      if (!empty($aggregation_filter)) {
        $aggregation_filter = $aggregation_filter[0];
      }
    }
    // Normal facet
    else {
      $aggregation_filter = $this->parseFilter($this->search_api_query->getFilter());
      if (!empty($aggregation_filter)) {
        $aggregation_filter = $aggregation_filter[0];
      }
    }

    return $aggregation_filter;
  }

  /**
   * Parse Search API filters.
   *
   * @param SearchApiQueryFilterInterface $query_filter
   * @param null $ignored_field
   * @return array|null
   * @throws Exception
   */
  protected function parseFilter(SearchApiQueryFilterInterface $query_filter, $ignored_field = null) {
    if (empty($query_filter)) {
      return null;
    }
    else {
      $conjunction = $query_filter->getConjunction();
      $filters = [];
      foreach ($query_filter->getFilters() as $filter_info) {
        $filter = null;

        // Simple filter [field_id, value, operator]
        if (is_array($filter_info)) {
          $filter_operator = str_replace('!=', '<>', $filter_info[2]);
          $filter_array = [
            'field_id' => $filter_info[0],
            'field_value' => $filter_info[1],
            'filter_operator' => $filter_operator,
          ];
          $this->verifyFilterConfiguration($filter_array, $ignored_field);
          $filter = $this->getFilter($filter_array);
          if (!empty($filter)) {
            $filters[] = $filter;
          }
        }
        elseif ($filter_info instanceof SearchApiQueryFilterInterface) {
          $nested_filters = $this->parseFilter($filter_info);
          if (!empty($nested_filters)) {
            $filters = array_merge($filters, $nested_filters);
          }
        }
      }
      $this->joinFilters($filters, $conjunction);
      return $filters;
    }
  }

  /**
   * Verify that a filter has the necessary array keys.
   *
   * @param $filter_array
   * @param $ignored_field
   * @return bool
   * @throws Exception
   */
  protected function verifyFilterConfiguration($filter_array, $ignored_field) {
    if (!array_key_exists('field_id', $filter_array) || !array_key_exists('filter_value', $filter_array)
      || !isset($filter_array['filter_operator'])) {
      throw new Exception(t('Incorrect filter criteria is using for searching!'));
    }
    $field_id = $filter_array['field_id'];
    // If field should be ignored, we skip.
    if ($field_id === $ignored_field) {
      return TRUE;
    }
    if (!isset($this->index_fields[$field_id])) {
      throw new Exception(t(':field_id Undefined field ! Incorrect filter criteria is using for searching!', array(':field_id' => $field_id)));
    }
    // Check operator.
    if (empty($filter_array['filter_operator'])) {
      throw new Exception(t('Empty filter operator for :field_id field! Incorrect filter criteria is using for searching!', array(':field_id' => $field_id)));
    }
  }

  /**
   * Get Elasticsearch aggregation queries for Search API filters.
   *
   * @param $filter_array
   * @return \Elastica\Query\AbstractQuery || null
   */
  protected function getFilter($filter_array) {
    // Handle "empty", "not empty" operators.
    if (!isset($filter_array['filter_value'])) {
      switch ($filter_array['filter_operator']) {
        case '<>':
          return new Exists($filter_array['field_id']);
        case '=':
          $query = new BoolQuery();
          $query->addMustNot(new Exists($filter_array['field_id']));
          return $query;
      }
    }
    // Normal filters
    else {
      switch ($filter_array['filter_value']) {
        case '>':
          return new RangeQuery($filter_array['field_id'], ['gt' => $filter_array['filter_value']]);
        case '>=':
          return new RangeQuery($filter_array['field_id'], ['gte' => $filter_array['filter_value']]);
        case '<':
          return new RangeQuery($filter_array['field_id'], ['lt' => $filter_array['filter_value']]);
        case '<=':
          return new RangeQuery($filter_array['field_id'], ['lte' => $filter_array['filter_value']]);
      }
    }

    return null;
  }

  /**
   * Join Elasticsearch aggregation queries.
   *
   * @param $filters
   * @param $conjunction
   * @throws Exception
   */
  protected function joinFilters(&$filters, $conjunction) {
    if (count($filters) > 1) {
      if ($conjunction === 'OR') {
        $query = new BoolQuery();
        $query->addShould($filters);
        $filters = array($query);
      }
      elseif ($conjunction === 'AND') {
        $query = new BoolQuery();
        $query->addMust($filters);
        $filters = array($query);
      }
      else {
        throw new Exception(t('Undefined conjunction :conjunction! Available values are "AND" and "OR"! Incorrect filter criteria is using for searching!',
          array(':conjunction!' => $conjunction)));
      }
    }
  }

  /**
   * Flatten keys for full text search.
   *
   * @param $keys
   * @param string $parse_mode
   * @param array $full_text_fields
   * @return string
   */
  protected function flattenKeys($keys, $parse_mode = '', $full_text_fields = array()) {
    $conjunction = isset($keys['#conjunction']) ? $keys['#conjunction'] : 'AND';
    $negation = !empty($keys['#negation']);
    $values = array();
    foreach (element_children($keys) as $key) {
      $value = $keys[$key];
      if (empty($value)) {
        continue;
      }
      if (is_array($value)) {
        $values[] = $this->flattenKeys($value);
      }
      elseif (is_string($value)) {
        // If parse mode is not "direct": quote the keyword.
        if ($parse_mode !== 'direct') {
          $value = '"' . $value . '"';
        }
        $values[] = $value;
      }
    }
    if (!empty($values)) {
      return ($negation === TRUE ? 'NOT ' : '') . '(' . implode(" {$conjunction} ", $values) . ')';
    }
    else {
      return '';
    }
  }

  /**
   * Get sorting configuration.
   *
   * @return array
   * @throws Exception
   */
  protected function getSortQuery() {
    $sort = [];
    foreach ($this->search_api_query->getSort() as $field_id => $direction) {
      $direction = drupal_strtolower($direction);
      if ($field_id === 'search_api_relevance') {
        $sort['_score'] = $direction;
      }
      elseif (isset($this->index_fields[$field_id])) {
        $sort[$field_id] = $direction;
      }
      else {
        throw new Exception(t('Incorrect Sorting!'));
      }
    }
    return $sort;
  }
}