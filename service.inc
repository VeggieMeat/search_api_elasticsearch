<?php
/**
 * @file
 * Provides a Elasticsearch-based service class for the Search API.
 */

/**
 * Search service class.
 */
class SearchApiElasticsearchService extends SearchApiAbstractService {

  /**
   * Elasticsearch Connection.
   */
  protected $elasticaClient = NULL;

  /**
   * Overrides __construct().
   */
  public function __construct(SearchApiServer $server) {
    parent::__construct($server);

    // TODO: use server options.
    // TODO: handle clusters.
    if (class_exists('Elastica_Client')) {
      $this->elasticaClient = new Elastica_Client(array(
        'host' => $this->getOption('host'),
        'port' => $this->getOption('port'),
      ));
    }
  }

  /**
   * Overrides configurationForm().
   */
  public function configurationForm(array $form, array &$form_state) {
    // Daemon settings.
    $form['daemon_settings'] = array(
      '#type' => 'fieldset',
      '#title' => t('Elasticsearch daemon settings'),
      '#tree' => FALSE,
    );

    // TODO: allow clusters.
    // Elasticsearch daemon host.
    $default = '127.0.0.1';
    $form['daemon_settings']['host'] = array(
      '#type' => 'textfield',
      '#title' => t('Host'),
      '#description' => t('Host to which the elasticsearch daemon will listen for this server. Default is %default.', array(
        '%default' => $default,
      )),
      '#required' => TRUE,
      '#default_value' => $this->getOption('host', $default),
      '#parents' => array('options', 'form', 'host'),
    );

    // Elasticsearch daemon port.
    $default = 9200;
    $form['daemon_settings']['port'] = array(
      '#type' => 'textfield',
      '#title' => t('Port'),
      '#description' => t('Port to which the elasticsearch daemon will listen for this server. Default is %default.', array(
        '%default' => $default,
      )),
      '#required' => TRUE,
      '#default_value' => $this->getOption('port', $default),
      '#parents' => array('options', 'form', 'port'),
    );

    // TODO: provide additional server options.
    return $form;
  }

  /**
   * Overrides configurationFormValidate().
   */
  public function configurationFormValidate(array $form, array &$values, array &$form_state) {
    // Daemon IP address.
    if (filter_var($values['host'], FILTER_VALIDATE_IP) === FALSE) {
      form_set_error('host', t('You must enter a valid IP address for the elasticsearch daemon.'));
    }

    // Daemon Port.
    if (filter_var($values['port'], FILTER_VALIDATE_INT, array('options' => array('min_range' => 0, 'max_range' => 65535))) === FALSE) {
      form_set_error('port', t('You must enter a valid Port (between 0 and 65535) for the elasticsearch daemon.'));
    }
  }

  /**
   * Overrides supportsFeature().
   */
  public function supportsFeature($feature) {
    $supported = drupal_map_assoc(array(
      'search_api_facets',
      'search_api_facets_operator_or',
      //'search_api_mlt',
    ));
    return isset($supported[$feature]);
  }

  /**
   * Overrides postCreate().
   */
  public function postCreate() {
  }

  /**
   * Overrides postUpdate().
   */
  public function postUpdate() {
    return FALSE;
  }

  /**
   * Overrides preDelete().
   */
  public function preDelete() {
  }

  /**
   * Overrides viewSettings().
   */
  public function viewSettings() {
    $output = array();

    $status = !empty($this->elasticaClient) ? $this->elasticaClient->getStatus()->getServerStatus() : NULL;
    $output['status'] = array(
      '#type' => 'item',
      '#title' => t('Elasticsearch daemon status'),
      '#markup' => '<div class="elasticsearch-daemon-status"><em>' . (!empty($status['ok']) ? 'running' : 'not running') . '</em></div>',
    );

    // Display settings.
    $form = $form_state = array();
    $option_form = $this->configurationForm($form, $form_state);
    $option_form['#title'] = t('Elasticsearch server settings');

    $element = $this->parseOptionFormElement($option_form, 'options');
    if (!empty($element)) {
      $settings = '';
      foreach ($element['option'] as $sub_element) {
        $settings .= $this->viewSettingElement($sub_element);
      }

      $output['settings'] = array(
        '#type' => 'fieldset',
        '#title' => $element['label'],
      );

      $output['settings'][] = array(
        '#type' => 'markup',
        '#markup' => '<div class="elasticsearch-server-settings">' . $settings . '</div>',
      );
    }

    return $output;
  }

  /**
   * Helper function. Parse an option form element.
   */
  protected function parseOptionFormElement($element, $key) {
    $children_keys = element_children($element);

    if (!empty($children_keys)) {
      $children = array();
      foreach ($children_keys as $child_key) {
        $child = $this->parseOptionFormElement($element[$child_key], $child_key);
        if (!empty($child)) {
          $children[] = $child;
        }
      }
      if (!empty($children)) {
        return array(
          'label' => isset($element['#title']) ? $element['#title'] : $key,
          'option' => $children,
        );
      }
    }
    elseif (isset($this->options[$key])) {
      return array(
        'label' => isset($element['#title']) ? $element['#title'] : $key,
        'option' => $key,
      );
    }

    return array();
  }

  /**
   * Helper function. Display a setting element.
   */
  protected function viewSettingElement($element) {
    $output = '';

    if (is_array($element['option'])) {
      $value = '';
      foreach ($element['option'] as $sub_element) {
        $value .= $this->viewSettingElement($sub_element);
      }
    }
    else {
      $value = $this->getOption($element['option']);
      $value = nl2br(check_plain(print_r($value, TRUE)));
    }
    $output .= '<dt><em>' . check_plain($element['label']) . '</em></dt>' . "\n";
    $output .= '<dd>' . $value . '</dd>' . "\n";

    return "<dl>\n{$output}</dl>";
  }

  /**
   * Overrides addIndex().
   */
  public function addIndex(SearchApiIndex $index) {
    $elastica_index = $this->getElasticaIndex($index);
    if (!empty($elastica_index)) {
      // TODO: handle index options.
      // TODO: handle response.
      $response = $elastica_index->create(array(
        'number_of_shards' => 1,
        'number_of_replicas' => 1,

      ), true);

      // Update mapping.
      $this->fieldsUpdated($index);
    }
  }

  /**
   * Overrides fieldsUpdated().
   */
  public function fieldsUpdated(SearchApiIndex $index) {
    $elastica_index = $this->getElasticaIndex($index);
    if (!empty($elastica_index)) {
      $elastica_type =  $elastica_index->getType($index->machine_name);

      // Create a new mapping.
      // TODO: check if we need to delete the previous mapping first.
      // TODO: shall we delete the items from the index?
      $mapping = new Elastica_Type_Mapping();
      $mapping->setType($elastica_type);

      $properties = array(
        'id' => array('type' => 'integer', 'include_in_all' => FALSE),
      );

      // Map index fields.
      foreach ($index->getFields() as $field_id => $field_data) {
        $properties[$field_id] = $this->getFieldMapping($field_data);
      }

      $mapping->setProperties($properties);

      // TODO: handle error.
      $mapping->send();
    }

    // Flag for re-indexing.
    return TRUE;
  }

  /**
   * Overrides removeIndex().
   */
  public function removeIndex($index) {
    $elastica_index = $this->getElasticaIndex($index);
    if (!empty($elastica_index)) {
      // TODO - handle errors.
      $response = $elastica_index->delete();
    }
  }

  /**
   * Overrides indexItems().
   */
  public function indexItems(SearchApiIndex $index, array $items) {
    $elastica_type = $this->getElasticaType($index);

    if (empty($elastica_type) || empty($items)) {
      return array();
    }

    $documents = array();
    foreach ($items as $id => $fields) {
      $data = array('id' => $id);
      foreach ($fields as $field_id => $field_data) {
        $data[$field_id] = $field_data['value'];
      }
      // TODO: error handling.
      $documents[] = new Elastica_Document($id, $data);
    }

    $elastica_type->addDocuments($documents);
    $elastica_type->getIndex()->refresh();

    return array_keys($items);
  }

  /**
   * Overrides deleteItems().
   */
  public function deleteItems($ids = 'all', SearchApiIndex $index = NULL) {
    if (empty($index)) {
      foreach ($this->getIndexes() as $index) {
        $this->deleteItems('all', $index);
      }
    }
    elseif ($ids === 'all') {
      // Faster to delete the index and recreate it.
      $this->removeIndex($index);
      $this->addIndex($index);
    }
    else {
      $elastica_type = $this->getElasticaType($index);
      if (!empty($elastica_type)) {
        $elastica_type->deleteIds($ids);
      }
    }
  }

  /**
   * Overrides search().
   */
  public function search(SearchApiQueryInterface $query) {
    // Results.
    $search_result = array('result count' => 0);

    $index = $query->getIndex();

    $elastica_type = $this->getElasticaType($index);

    // Check elasticsearch index.
    if (empty($elastica_type)) {
      return $search_result;
    }

    // Indexed fields.
    $index_fields = $index->getFields();

    // Query options.
    $query_options = $query->getOptions();

    // Range.
    $query_offset = empty($query_options['offset']) ? 0 : $query_options['offset'];
    $query_limit = empty($query_options['limit']) ? 10 : $query_options['limit'];

    // Query string.
    $query_search_string = NULL;

    // Query filter.
    $query_search_filter = NULL;

    // Full text search.
    $keys = $query->getKeys();
    if (!empty($keys)) {
      if (is_string($keys)) {
        $keys = array($keys);
      }

      // Full text fields in which to perform the search.
      $query_full_text_fields = $query->getFields();

      // Query string
      $search_string = $this->flattenKeys($keys, $query_options['parse mode']);

      if (!empty($search_string)) {
        $query_search_string = new Elastica_Query_QueryString($search_string);
      }
    }

    // Filters.
    $parsed_query_filters = $this->parseFilter($query->getFilter(), $index_fields);
    if (!empty($parsed_query_filters)) {
      $query_search_filter = $parsed_query_filters[0];
    }

    // Main query.
    $elastica_query = new Elastica_Query();
    $elastica_query->setFrom($query_offset);
    $elastica_query->setSize($query_limit);

    // Build the query.
    if (!empty($query_search_string) && !empty($query_search_filter)) {
      $elastica_query->setQuery(new Elastica_Query_Filtered($query_search_string, $query_search_filter));
    }
    elseif (!empty($query_search_string)) {
      $elastica_query->setQuery($query_search_string);
    }
    elseif (!empty($query_search_filter)) {
      $elastica_query->setFilter($query_search_filter);
    }

    // Sort.
    $sort = array();
    foreach ($query->getSort() as $field_id => $direction) {
      $direction = drupal_strtolower($direction);
      if ($field_id === 'search_api_relevance') {
        $sort['_score'] = $direction;
      }
      elseif (isset($index_fields[$field_id])) {
        $sort[$field_id] = $direction;
      }
      else {
        // TODO: handle error.
      }
    }
    if (!empty($sort)) {
      $elastica_query->setSort($sort);
    }


    // SEARCH API FACETS.
    $facets = $query->getOption('search_api_facets');
    if (!empty($facets)) {
      // Active search corresponding to this index.
      $searcher = key(facetapi_get_active_searchers());
      // Get the FacetApiAdpater for this searcher.
      $adapter = isset($searcher) ? facetapi_adapter_load($searcher) : NULL;

      // Date gaps.
      $gap_weight = array('YEAR' => 2, 'MONTH' => 1, 'DAY' => 0);

      // Loop trough facets.
      foreach ($facets as $facet_id => $facet_info) {
        $facet = NULL;
        $field_id = $facet_info['field'];

        // Skip if not recognized as a known field.
        if (!isset($index_fields[$field_id])) {
          continue;
        }

        $facet_limit = !empty($facet_info['limit']) ? $facet_info['limit'] : -1;
        $facet_missing = $facet_info['missing'];

        $field_type = search_api_extract_inner_type($index_fields[$field_id]['type']);

        // TODO: handle different types (GeoDistance and so on).
        if ($field_type === 'date') {
          $gaps = array();
          $date_gap = 'YEAR';

          // Get the date granularity.
          if (isset($adapter)) {
            // Get the current date gap from the active date filters.
            $active_items = $adapter->getActiveItems(array('name' => $facet_id));
            if (!empty($active_items)) {
              foreach ($active_items as $active_item) {
                $value = $active_item['value'];
                if (strpos($value, ' TO ') > 0) {
                  list($date_min, $date_max) = explode(' TO ', str_replace(array('[', ']'), '', $value), 2);
                  $gap = self::getDateGap($date_min, $date_max, FALSE);
                  if (isset($gap_weight[$gap])) {
                    $gaps[] = $gap_weight[$gap];
                  }
                }
              }
              if (!empty($gaps)) {
                // Minimum gap.
                $date_gap = array_search(min($gaps), $gap_weight);
              }
            }
          }

          switch ($date_gap) {
            // Already a selected YEAR, we want the months.
            case 'YEAR':
              $date_interval = 'month';
              break;

            // Already a selected MONTH, we want the days.
            case 'MONTH':
              $date_interval = 'day';
              break;

            // Already a selected DAY, we want the hours and so on.
            case 'DAY':
              $date_interval = 'hour';
              break;

            // By default we return result counts by year.
            default:
              $date_interval = 'year';
          }

          $facet = new Elastica_Facet_DateHistogram($facet_id);
          $facet->setInterval($date_interval);
          // Maybe get php timezone?
          $facet->setTimezone('UTC');
          // Use factor 1000 as we store dates as seconds from epoch
          // not milliseconds.
          $facet->setParam('factor', 1000);
        }
        else {
          $facet = new Elastica_Facet_Terms($facet_id);
          // We may want missing facets.
          $facet->setAllTerms($facet_missing);
        }

        // Add the facet.
        if (!empty($facet)) {
          // Set the field.
          $facet->setField($field_id);


          // OR facet. We remove filters affecting the assiociated field.
          // TODO: distinguish between normal filters and facet filters.
          // See http://drupal.org/node/1390598.
          if (isset($facet_info['operator']) && drupal_strtolower($facet_info['operator']) == 'or') {
            $facet_search_filter = $this->parseFilter($query->getFilter(), $index_fields, $field_id);
            if (!empty($facet_search_filter)) {
              $facet_search_filter = $facet_search_filter[0];
            }
          }
          // Normal facet, we just use the main query filters.
          else {
            $facet_search_filter = $query_search_filter;
          }

          // Filter the facet.
          if (!empty($facet_search_filter)) {
            $facet->setFilter($facet_search_filter);
          }

          // Limit the number of returned entries.
          if ($facet_limit > 0) {
            $facet->setSize($facet_limit);
          }

          $elastica_query->addFacet($facet);
        }
      }
    }

    // Perform search.
    $response = $elastica_type->search($elastica_query);

    // Parse response.
    if (!empty($response)) {
      $search_result['result count'] = $response->getTotalHits();

      // Parse results.
      foreach ($response->getResults() as $result) {
        $id = $result->getId();

        $search_result['results'][$id] = array(
          'id' => $id,
          'score' => $result->getScore(),
          'fields' => $result->getSource(),
        );
      }

      // Parse facets.
      if (!empty($facets) && $response->hasFacets()) {
        foreach ($response->getFacets() as $facet_id => $facet_data) {
          if (isset($facets[$facet_id])) {
            $facet_info = $facets[$facet_id];
            $facet_min_count = $facet_info['min_count'];

            $field_id = $facet_info['field'];
            $field_type = search_api_extract_inner_type($index_fields[$field_id]['type']);

            // TODO: handle different types (GeoDistance and so on).
            if ($field_type === 'date') {
              foreach ($facet_data['entries'] as $entry) {
                if ($entry['count'] >= $facet_min_count) {
                  // Divide time by 1000 as we want seconds from epoch
                  // not milliseconds.
                  $search_result['search_api_facets'][$facet_id][] = array(
                    'count' => $entry['count'],
                    'filter' => '"' . ($entry['time'] / 1000) . '"',
                  );
                }
              }
            }
            else {
              foreach ($facet_data['terms'] as $term) {
                if ($term['count'] >= $facet_min_count) {
                  $search_result['search_api_facets'][$facet_id][] = array(
                    'count' => $term['count'],
                    'filter' => '"' . $term['term'] . '"',
                  );
                }
              }
            }
          }
        }
      }
    }

    return $search_result;
  }

  /**
   * Recursively parse Search API filters.
   */
  protected function parseFilter(SearchApiQueryFilter $query_filter, $index_fields, $ignored_field_id = '') {
    if (empty($query_filter)) {
      return NULL;
    }
    else {
      $conjunction = $query_filter->getConjunction();

      $filters = array();

      foreach ($query_filter->getFilters() as $filter_info) {
        $filter = NULL;

        // Simple filter [field_id, value, operator].
        if (is_array($filter_info)) {
          $field_id = $filter_info[0];
          $filter_value = $filter_info[1];
          $filter_operator = str_replace('!=', '<>', $filter_info[2]);

          // Check field.
          if (!isset($index_fields[$field_id])) {
            // TODO: handle error.
            return NULL;
          }

          // Check operator.
          if (empty($filter_operator)) {
            // TODO: handle error.
            return NULL;
          }

          // If field should be ignored, we skip.
          if ($field_id === $ignored_field_id) {
            continue;
          }

          // Handles "empty", "not empty" operators.
          if (!isset($filter_value)) {
            switch ($filter_operator) {
              case '=':
                $filter = new Elastica_Filter_Exists($field_id);
                break;

              case '<>':
                $filter = new Elastica_Filter_Not(new Elastica_Filter_Exists($field_id));
                break;

              default:
                // TODO: handle error.
                return NULL;
            }
          }
          // Normal filters.
          else {
            switch ($filter_operator) {
              case '=':
                $filter = new Elastica_Filter_Term(array($field_id => $filter_value));
                break;

              case '<>':
                $filter = new Elastica_Filter_Not(new Elastica_Filter_Term(array($field_id => $filter_value)));
                break;

              case '>':
                $filter = new Elastica_Filter_Range($field_id, array(
                  'from' => $filter_value,
                  'to' => NULL,
                  'include_lower' => FALSE,
                  'include_upper' => FALSE,
                ));
                break;

              case '>=':
                $filter = new Elastica_Filter_Range($field_id, array(
                  'from' => $filter_value,
                  'to' => NULL,
                  'include_lower' => TRUE,
                  'include_upper' => FALSE,
                ));
                break;

              case '<':
                $filter = new Elastica_Filter_Range($field_id, array(
                  'from' => NULL,
                  'to' => $filter_value,
                  'include_lower' => FALSE,
                  'include_upper' => FALSE,
                ));
                break;

              case '<=':
                $filter = new Elastica_Filter_Range($field_id, array(
                  'from' => NULL,
                  'to' => $filter_value,
                  'include_lower' => FALSE,
                  'include_upper' => TRUE,
                ));
                break;

              default:
                // TODO: handle error.
                return NULL;
            }
          }

          if (!empty($filter)) {
            $filters[] = $filter;
          }
        }
        // Nested filters.
        elseif ($filter_info instanceof SearchApiQueryFilter) {
          $nested_filters = $this->parseFilter($filter_info, $index_fields, $ignored_field_id);
          // TODO: handle error.
          if (!empty($nested_filters)) {
            $filters = array_merge($filters, $nested_filters);
          }
        }
      }

      if (count($filters) > 1) {
        if ($conjunction === 'OR') {
          $filter = new Elastica_Filter_Or();
          $filter->setFilters($filters);
          $filters = array($filter);
        }
        elseif ($conjunction === 'AND') {
          $filter = new Elastica_Filter_And();
          $filter->setFilters($filters);
          $filters = array($filter);
        }
        else {
          // TODO: handle error.
          return NULL;
        }
      }

      return $filters;
    }
  }


  /**
   * Return a full text search query.
   *
   * TODO: better handling of parse modes.
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
   * Helper function. Returns the elasticsearch name of an index.
   */
  public function getIndexName(SearchApiIndex $index) {
    global $databases;

    $site_database = $databases['default']['default']['database'];

    $index_machine_name = is_string($index) ? $index : $index->machine_name;

    return self::escapeName('elasticsearch_index_' . $site_database . '_' . $index_machine_name);
  }

  /**
   * Helper function. Escape a field or index name.
   *
   * Force names to be strictly alphanumeric-plus-underscore.
   */
  public static function escapeName($name) {
    return preg_replace('/[^A-Za-z0-9_]+/', '', $name);
  }

  /**
   * Helper function. Get an Elastica index.
   */
  public function getElasticaIndex(SearchApiIndex $index) {
    if (!empty($index) && !empty($this->elasticaClient)) {
      $index_name = $this->getIndexName($index);
      // TODO: catch exception.
      return $this->elasticaClient->getIndex($index_name);
    }
    return NULL;
  }

  /**
   * Helper function. Get the corresponding Elastica type.
   */
  public function getElasticaType(SearchApiIndex $index) {
    $elastica_index = $this->getElasticaIndex($index);
    return !empty($elastica_index) ? $elastica_index->getType($index->machine_name) : NULL;
  }

  /**
   * Helper function. Get the elasticsearch mapping for a field.
   */
  public function getFieldMapping($field) {
    $type = search_api_extract_inner_type($field['type']);

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

      default:
        return NULL;
    }
  }

  /**
   * Helper function. Return date gap from two dates or timestamps.
   *
   * @see facetapi_get_timestamp_gap()
   */
  protected static function getDateGap($min, $max, $timestamp = TRUE) {
    if ($timestamp !== TRUE) {
      $min = strtotime($min);
      $max = strtotime($max);
    }

    if (empty($min) || empty($max)) {
      return 'DAY';
    }

    $diff = $max - $min;

    switch (TRUE) {
      case ($diff > 86400 * 365):
        return 'NONE';

      case ($diff > 86400 * gmdate('t', $min)):
        return 'YEAR';

      case ($diff > 86400):
        return 'MONTH';

      default:
        return 'DAY';
    }
  }

  /**
   * Helper function. Return server options.
   */
  public function getOptions() {
    return $this->options;
  }

  /**
   * Helper function. Return a server option.
   */
  public function getOption($option, $default = NULL) {
    $options = $this->getOptions();
    return isset($options[$option]) ? $options[$option] : $default;
  }

}
