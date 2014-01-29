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

    if (class_exists('Elastica\Client')) {
      $config = array();
      $count_options = array();

      $options = $this->getOptions();

      if (isset($options) && !empty($options)) {
        $count_options = $options;
        if (isset($count_options['facet_limit'])) {
          unset($count_options['facet_limit']);
        }

        if (count($count_options) > 1) {
          foreach ($count_options as $id => $option) {
            $config[] = $option;
          }

          try {
            $this->elasticaClient = new Elastica\Client(array('servers' => $config));
          }
          catch (Exception $e) {
            watchdog('Elastic Search', check_plain($e->getMessage()), array(), WATCHDOG_ERROR);
            drupal_set_message(check_plain($e->getMessage()), 'error');
            return FALSE;
          }
        }
        else {
          $config = reset($count_options);
          if (module_exists('search_api_facetapi')) {
            if (isset($options['facet_limit'])) {
              $config['facet_limit'] = $options['facet_limit'];
            }
          }
          try {
            $this->elasticaClient = new Elastica\Client($config);
          }
          catch (Exception $e) {
            watchdog('Elastic Search', check_plain($e->getMessage()), array(), WATCHDOG_ERROR);
            drupal_set_message(check_plain($e->getMessage()), 'error');
            return FALSE;
          }
        }
      }
    }
    else {
      watchdog('Elastica Search', 'Elastica Client does not exist', array(), WATCHDOG_ERROR);
      return FALSE;
    }
  }

  /**
   * Overrides configurationForm().
   */
  public function configurationForm(array $form, array &$form_state) {
    $options = $this->options + array(
      'host' => '127.0.0.1',
      'port' => 9200,
      'path' => '',
      'url' => NULL,
      'transport' => 'Http',
      'persistent' => TRUE,
      'timeout' => 300,
      'log' => FALSE,
      'retryOnConflict' => 0,
    );

    // Daemon settings.
    $form['daemon_settings'] = array(
      '#type' => 'fieldset',
      '#title' => t('Elasticsearch client settings'),
      '#tree' => TRUE,
      '#prefix' => '<div id="elasticsearch-ajax-wrapper">',
      '#suffix' => '</div>',
    );

    $delta = 1;
    $i = 1;
    if (isset($form_state['values']['options']['form']) && !empty($form_state['values']['options']['form']) && !isset($form_state['values']['remove_delta'])) {
      unset($form_state['values']['options']['form']['add_more']);
      if (isset($form_state['values']['options']['form']['facet_limit'])) {
        unset($form_state['values']['options']['form']['facet_settings']);
        unset($form_state['values']['options']['form']['facet_limit']);
      }

      $delta = count($form_state['values']['options']['form'])+1;
    }
    elseif (isset($form_state['values']['remove_delta'])) {
      unset($form_state['values']['options']['form']['add_more']);
      if (isset($form_state['values']['options']['form']['facet_limit'])) {
        unset($form_state['values']['options']['form']['facet_settings']);
        unset($form_state['values']['options']['form']['facet_limit']);
      }

      $delta = count($form_state['values']['options']['form']);
    }
    elseif (isset($this->options) && !empty($this->options)) {
      if (isset($this->options['facet_limit'])) {
        unset($this->options['facet_limit']);
      }
      $delta = count($this->options);
    }

    for ($c = 0; $c < $delta; $c++) {
      if (isset($form_state['values']['remove_delta']) && $c == $form_state['values']['remove_delta']) {
        unset($form_state['values']['options']['form'][$form_state['values']['remove_delta']]);
        continue;
      }
      else {
        // Daemon settings.
        $form['daemon_settings']['fieldset'][$c] = array(
          '#type' => 'fieldset',
          '#title' => t('Node :id', array(':id' => $i)),
          '#tree' => TRUE,
          '#collapsible' => TRUE,
          '#collapsed' => TRUE,
        );

        // Elasticsearch daemon host.
        $form['daemon_settings']['fieldset'][$c]['host'] = array(
          '#type' => 'textfield',
          '#title' => t('Host'),
          '#description' => t('Host to which the elasticsearch daemon will listen for this server. Default is %default.', array(
            '%default' => $options['host'],
          )),
          '#required' => TRUE,
          '#default_value' => ($options[$c]['host']) ? $options[$c]['host'] : $options['host'],
          '#parents' => array('options', 'form', $c, 'host'),
        );

        // Elasticsearch daemon port.
        $form['daemon_settings']['fieldset'][$c]['port'] = array(
          '#type' => 'textfield',
          '#title' => t('Port'),
          '#description' => t('Port to which the elasticsearch daemon will listen for this server. Default is %default.', array(
            '%default' => $options['port'],
          )),
          '#required' => TRUE,
          '#default_value' => ($options[$c]['port']) ? $options[$c]['port'] : $options['port'],
          '#parents' => array('options', 'form', $c, 'port'),
        );

        // Elasticsearch daemon path.
        $form['daemon_settings']['fieldset'][$c]['path'] = array(
          '#type' => 'textfield',
          '#title' => t('ElasticSearch path prefix'),
          '#description' => t('Normally empty. Use when you have remapped the ElasticSearch server API path.'),
          '#required' => FALSE,
          '#default_value' => ($options[$c]['path']) ? $options[$c]['path'] : $options['path'],
          '#parents' => array('options', 'form', $c, 'path'),
        );

        // Elasticsearch daemon URL.
        $form['daemon_settings']['fieldset'][$c]['url'] = array(
          '#type' => 'textfield',
          '#title' => t('ElasticSearch url'),
          '#description' => t('Normally empty. Use instead of host/port when you have remapped the ElasticSearch server API url.'),
          '#required' => FALSE,
          '#default_value' => ($options[$c]['url']) ? $options[$c]['url'] : $options['url'],
          '#parents' => array('options', 'form', $c, 'url'),
        );

        // Elasticsearch daemon transport.
        $form['daemon_settings']['fieldset'][$c]['transport'] = array(
          '#type' => 'select',
          '#title' => t('Select transport'),
          '#description' => t('Transport to connect to this elasticsearch server.'),
          '#options' => array(
            'Http' => 'HTTP',
            'Https' => 'HTTPS',
            'Memcache' => 'Memcache',
            'Null' => 'NULL',
          ),
          '#default_value' => ($options[$c]['transport']) ? $options[$c]['transport'] : $options['transport'],
          '#parents' => array('options', 'form', $c, 'transport'),
        );

        // Elasticsearch daemon persistent.
        $form['daemon_settings']['fieldset'][$c]['persistent'] = array(
          '#type' => 'checkbox',
          '#title' => t('Persistent connection'),
          '#description' => t('Use persistent connection when connecting to this node.'),
          '#default_value' => (isset($options[$c]['persistent'])) ? $options[$c]['persistent'] : $options['persistent'],
          '#parents' => array('options', 'form', $c, 'persistent'),
        );

        // Elasticsearch daemon timeout.
        $form['daemon_settings']['fieldset'][$c]['timeout'] = array(
          '#type' => 'textfield',
          '#title' => t('Timeout in ms'),
          '#description' => t('Timeout in ms for waiting this elastic server to respond'),
          '#default_value' => ($options[$c]['timeout']) ? $options[$c]['timeout'] : $options['timeout'],
          '#parents' => array('options', 'form', $c, 'timeout'),
        );

        // Elasticsearch daemon log.
        $form['daemon_settings']['fieldset'][$c]['log'] = array(
          '#type' => 'checkbox',
          '#title' => t('Log'),
          '#description' => t('Log this elasticsearch server queries to the default log.'),
          '#default_value' => ($options[$c]['log']) ? $options[$c]['log'] : $options['log'],
          '#parents' => array('options', 'form', $c, 'log'),
        );

        // Elasticsearch daemon retryOnConflict.
        $form['daemon_settings']['fieldset'][$c]['retryOnConflict'] = array(
          '#type' => 'textfield',
          '#title' => t('retryOnConflict'),
          '#description' => t('Sets the number of retries of a version conflict occurs because the document was updated between getting it and updating it.'),
          '#default_value' => ($options[$c]['retryOnConflict']) ? $options[$c]['retryOnConflict'] : $options['retryOnConflict'],
          '#parents' => array('options', 'form', $c, 'retryOnConflict'),
        );

        if ((!isset($form_state['values']['remove_delta']) && $delta > 1) || (isset($form_state['values']['remove_delta']) && $delta > 2)) {
          // Elasticsearch daemon retryOnConflict.
          $form['daemon_settings']['fieldset'][$c]['remove_node'] = array(
            '#type' => 'submit',
            '#value' => t('Remove node') . ' ' . $i,
            '#submit' => array('_search_api_elasticsearch_configuration_form_remove_custom'),
            '#ajax' => array(
              'callback' => '_search_api_elasticsearch_configuration_form_remove_ajax',
              'wrapper' => 'elasticsearch-ajax-wrapper',
              'method' => 'replace',
              'effect' => 'fade',
            ),
            '#remove_delta' => $c,
            '#parents' => array('options', 'form', $c, 'remove_node'),
          );
        }
      }
      $i++;
    }

    // Elasticsearch daemon retryOnConflict.
    $form['add_more'] = array(
      '#type' => 'submit',
      '#value' => t('+'),
      '#submit' => array('_search_api_elasticsearch_configuration_form_submit_custom'),
      '#ajax' => array(
        'callback' => '_search_api_elasticsearch_configuration_form_ajax',
        'wrapper' => 'elasticsearch-ajax-wrapper',
        'method' => 'replace',
        'effect' => 'fade',
      ),
    );

    if (module_exists('search_api_facetapi')) {
      // Facet settings.
      $form['facet_settings'] = array(
        '#type' => 'fieldset',
        '#title' => t('Elasticsearch facet settings'),
        '#tree' => FALSE,
      );

      // Elasticsearch facet limit.
      $default = 10;
      $form['facet_settings']['facet_limit'] = array(
        '#type' => 'textfield',
        '#title' => t('Facet limit'),
        '#description' => t("Maximum number of facet elements to be returned by the server if 'no limit' is selected as hard limit is the facet option. Default is %default.", array(
          '%default' => $default,
        )),
        '#required' => TRUE,
        '#default_value' => (isset($options['facet_limit'])) ? $options['facet_limit'] : $default,
        '#parents' => array('options', 'form', 'facet_limit'),
      );
    }

    return $form;
  }

  /**
   * Overrides configurationFormValidate().
   */
  public function configurationFormValidate(array $form, array &$values, array &$form_state) {
    unset($values['add_more']);
    $count_nodes = count($values);

    // PATCH - Validation breaks when trying to create new server without the Search API Facetapi module installed
    if (module_exists('search_api_facetapi')) {
      // Facet limit.
      if (filter_var($values['facet_limit'], FILTER_VALIDATE_INT, array('options' => array('min_range' => 0))) === FALSE) {
        form_set_error('options][form][facet_limit', t('You must enter a positive integer for the elasticsearch facet limit.'));
      }
    }

    foreach ($values as $i => $setting) {
      if ($i != 'facet_limit') {
        // Daemon IP address.
        if (filter_var($values[$i]['host'], FILTER_VALIDATE_IP) === FALSE) {
          form_set_error('options][form]' . $i . '[host', t('You must enter a valid IP address for the elasticsearch daemon.'));
        }

        // Daemon Port.
        if (filter_var($values[$i]['port'], FILTER_VALIDATE_INT, array('options' => array('min_range' => 0, 'max_range' => 65535))) === FALSE) {
          form_set_error('options][form]' . $i . '[port', t('You must enter a valid Port (between 0 and 65535) for the elasticsearch daemon.'));
        }

        $values[$i]['path'] = $this->setPath($values[$i]['path']);
      }
    }
  }

  /**
   * Overrides configurationFormSubmit().
   */
  public function configurationFormSubmit(array $form, array &$values, array &$form_state) {
    $facet_limit = '';

    if (isset($values['facet_limit'])) {
      $facet_limit = $values['facet_limit'];
      unset($values['facet_limit']);
    }

    $values = array_values($values);
    $values['facet_limit'] = $facet_limit;
    $this->options = $values;
  }

  /**
   * Overrides supportsFeature().
   */
  public function supportsFeature($feature) {
    $supported = drupal_map_assoc(array(
          'search_api_facets',
          'search_api_facets_operator_or',
           'search_api_autocomplete',
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

    try {
      $status = !empty($this->elasticaClient) ? $this->elasticaClient->getStatus()->getServerStatus() : NULL;
    }
    catch (Exception $e) {
      watchdog('Elastic Search', check_plain($e->getMessage()), array(), WATCHDOG_ERROR);
      drupal_set_message(check_plain($e->getMessage()), 'error');
      drupal_set_message(t('No connection to the Elasticsearch server.'), 'error');
    }
    $output['status'] = array(
      '#type' => 'item',
      '#title' => t('Elasticsearch daemon status'),
      '#markup' => '<div class="elasticsearch-daemon-status"><em>' . (!empty($status['ok']) ? 'running' : 'One or more nodes not running') . '</em></div>',
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
      try {
        $response = $elastica_index->create($index->options, TRUE);
      }
      catch (Exception $e) {
        watchdog('Elastic Search', check_plain($e->getMessage()), array(), WATCHDOG_ERROR);
        drupal_set_message(check_plain($e->getMessage()), 'error');
        return FALSE;
      }

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
      $elastica_type = $elastica_index->getType($index->machine_name);

      // Create a new mapping.
      // TODO: check if we need to delete the previous mapping first.
      // TODO: shall we delete the items from the index?
      $mapping = new Elastica\Type\Mapping();
      $mapping->setType($elastica_type);
      $mapping->setParam('_all', array('enabled' => FALSE));
      $properties = array(
        'id' => array('type' => 'integer', 'include_in_all' => FALSE),
      );
      try {
        // Map index fields.
        foreach ($index->getFields() as $field_id => $field_data) {
          $properties[$field_id] = $this->getFieldMapping($field_data);
        }

        $mapping->setProperties($properties);

        $mapping->send();
      }
      catch (Exception $e) {
        watchdog('Elastic Search', check_plain($e->getMessage()), array(), WATCHDOG_ERROR);
        drupal_set_message(check_plain($e->getMessage()), 'error');
        drupal_set_message(t('Fields are not re-index'), 'error');
        return FALSE;
      }
    }

    // Flag for re-indexing.
    return TRUE;
  }

  /**
   * Overrides removeIndex().
   */
  public function removeIndex($index) {
    $elastica_index = $this->getElasticaIndex($index);
    // PATCH - Only delete the index's data if the index isn't read-only.
    if (!empty($elastica_index) && $index->read_only != 1) {
      try {
        $response = $elastica_index->delete();
        return $response;
      }
      catch (Exception $e) {
        watchdog('Elastic Search', check_plain($e->getMessage()), array(), WATCHDOG_ERROR);
        drupal_set_message(check_plain($e->getMessage()), 'error');
        return FALSE;
      }
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
        if (is_array($field_data['value'])) {
          $data[$field_id] = array();
          foreach ($field_data['value'] as $token) {
            if (isset($token['value'])) {
              $data[$field_id][] = $token['value'];
            }
          }
        }
        else {
          $data[$field_id] = $field_data['value'];
        }
      }

      $documents[] = new Elastica\Document($id, $data);
    }
    try {
      $elastica_type->addDocuments($documents);
    }
    catch (Exception $e) {
      watchdog('Elastic Search', check_plain($e->getMessage()), array(), WATCHDOG_ERROR);
      drupal_set_message(check_plain($e->getMessage()), 'error');
    }
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

    // Get index
    $index = $query->getIndex();

    // Get index type
    $elastica_type = $this->getElasticaType($index);

    // Check elasticsearch index.
    if (empty($elastica_type)) {
      return $search_result;
    }

    // Build Elastica query.
    $elastica_query = $this->buildSearchQuery($query);

    // Add facets.
    $this->addSearchFacets($elastica_query, $query);

    // Perform search.
    $response = $elastica_type->search($elastica_query);

    // Parse response.
    if (!empty($response)) {
      $search_result = $this->parseSearchResponse($response, $query);
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

      try {
        foreach ($query_filter->getFilters() as $filter_info) {
          $filter = NULL;

          // Simple filter [field_id, value, operator].
          if (is_array($filter_info)) {
            $filter_assoc = $this->getAssociativeFilter($filter_info);
            $this->correctFilter($filter_assoc, $index_fields, $ignored_field_id);
            // Check field.
            $filter = $this->getFilter($filter_assoc);

            if (!empty($filter)) {
              $filters[] = $filter;
            }
          }
          // Nested filters.
          elseif ($filter_info instanceof SearchApiQueryFilter) {
            $nested_filters = $this->parseFilter($filter_info, $index_fields, $ignored_field_id);
            // TODO: handle error. - here is unnecessary cause in if we thow exceptions and this is still in try{}  .
            if (!empty($nested_filters)) {
              $filters = array_merge($filters, $nested_filters);
            }
          }
        }
        $filters = $this->setFiltersConjunction($filters, $conjunction);
      }
      catch (Exception $e) {
        watchdog('Elastic Search', check_plain($e->getMessage()), array(), WATCHDOG_ERROR);
        drupal_set_message(check_plain($e->getMessage()), 'error');
      }

      return $filters;
    }
  }

  /**
   * Get filter by associative array/
   */
  protected function getFilter(array $filter_assoc) {
    // Handles "empty", "not empty" operators.
    if (!isset($filter_assoc['filter_value'])) {
      switch ($filter_assoc['filter_operator']) {
        case '<>':
          $filter = new Elastica\Filter\Exists($field_id);
          break;

        case '=':
          $filter = new Elastica\Filter\BoolNot(new Elastica\Filter\Exists($filter_assoc['field_id']));
          break;

        default:
          throw new Exception(t('Value is empty for :field_id! Incorrect filter criteria is using for searching!', array(':field_id' => $filter_assoc['field_id'])));
      }
    }
    // Normal filters.
    else {
      switch ($filter_assoc['filter_operator']) {
        case '=':
          $filter = new Elastica\Filter\Term(array($filter_assoc['field_id'] => $filter_assoc['filter_value']));
          break;

        case '<>':
          $filter = new Elastica\Filter\BoolNot(new Elastica\Filter\Term(array($filter_assoc['field_id'] => $filter_assoc['filter_value'])));
          break;

        case '>':
          $filter = new Elastica\Filter\Range($filter_assoc['field_id'], array(
                'from' => $filter_assoc['filter_value'],
                'to' => NULL,
                'include_lower' => FALSE,
                'include_upper' => FALSE,
              ));
          break;

        case '>=':
          $filter = new Elastica\Filter\Range($filter_assoc['field_id'], array(
                'from' => $filter_assoc['filter_value'],
                'to' => NULL,
                'include_lower' => TRUE,
                'include_upper' => FALSE,
              ));
          break;

        case '<':
          $filter = new Elastica\Filter\Range($filter_assoc['field_id'], array(
                'from' => NULL,
                'to' => $filter_assoc['filter_value'],
                'include_lower' => FALSE,
                'include_upper' => FALSE,
              ));
          break;

        case '<=':
          $filter = new Elastica\Filter\Range($filter_assoc['field_id'], array(
                'from' => NULL,
                'to' => $filter_assoc['filter_value'],
                'include_lower' => FALSE,
                'include_upper' => TRUE,
              ));
          break;

        default:
          throw new Exception(t('Undefined operator :field_operator for :field_id field! Incorrect filter criteria is using for searching!',
                  array(':field_operator' => $filter_assoc['filter_operator'], ':field_id' => $filter_assoc['field_id'])));
      }
    }

    return $filter;
  }

  /**
   * Helper function that return associative array  of filters info.
   */
  public function getAssociativeFilter(array $filter_info) {

    $filter_operator = str_replace('!=', '<>', $filter_info[2]);
    return array(
      'field_id' => $filter_info[0],
      'filter_value' => $filter_info[1],
      'filter_operator' => $filter_operator,
    );
  }

  /**
   * Helper function thaht set filters conjunction
   */
  protected function setFiltersConjunction(&$filters, $conjunction) {

    if (count($filters) > 1) {
      if ($conjunction === 'OR') {
        $filter = new Elastica\Filter\BoolOr();
        $filter->setFilters($filters);
        $filters = array($filter);
      }
      elseif ($conjunction === 'AND') {
        $filter = new Elastica\Filter\BoolAnd();
        $filter->setFilters($filters);
        $filters = array($filter);
      }
      else {
        throw new Exception(t('Undefined conjunction :conjunction! Available values are :avail_conjunction! Incorrect filter criteria is using for searching!',
                array(':conjunction!' => $conjunction, ':avail_conjunction' => $conjunction)));
        return NULL;
      }
    }
    return $filters;
  }

  /**
   * Helper function that check if filter is set correct.
   */
  protected function correctFilter($filter_assoc, $index_fields, $ignored_field_id = '') {
    if (!isset($filter_assoc['field_id']) || !isset($filter_assoc['filter_value'])
        || !isset($filter_assoc['filter_operator'])) {
      throw new Exception(t('Incorrect filter criteria is using for searching!'));
    }

    $field_id = $filter_assoc['field_id'];
    if (!isset($index_fields[$field_id])) {
      throw new Exception(t(':field_id Undefined field ! Incorrect filter criteria is using for searching!', array(':field_id' => $field_id)));
    }

    // Check operator.
    if (empty($filter_assoc['filter_operator'])) {
      throw new Exception(t('Empty filter operator for :field_id field! Incorrect filter criteria is using for searching!', array(':field_id' => $field_id)));
    }

    // If field should be ignored, we skip.
    if ($field_id === $ignored_field_id) {
      return TRUE;
    }

    return TRUE;
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
      try {
        $elastica_index = $this->elasticaClient->getIndex($index_name);
        return $elastica_index;
      }
      catch (Exception $e) {
        watchdog('Elastic Search', check_plain($e->getMessage()), array(), WATCHDOG_ERROR);
        drupal_set_message(check_plain($e->getMessage()), 'error');
      }
    }
    return NULL;
  }

  /**
   * Helper function. Get the corresponding Elastica type.
   */
  public function getElasticaType(SearchApiIndex $index) {
    $elastica_index = $this->getElasticaIndex($index);
    return!empty($elastica_index) ? $elastica_index->getType($index->machine_name) : NULL;
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

  /**
   * Helper function. Return index fields.
   */
  public function getIndexFields(SearchApiQueryInterface $query) {
    $index = $query->getIndex();
    $index_fields = $index->getFields();
    return $index_fields;
  }

  /**
   * Helper function build search query().
   */
  protected function buildSearchQuery(SearchApiQueryInterface $query) {
    // Query options.
    $query_options = $this->getSearchQueryOptions($query);

    // Main query.
    $elastica_query = new Elastica\Query();
    $elastica_query->setFrom($query_options['query_offset']);
    $elastica_query->setSize($query_options['query_limit']);

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

    // Sort
    if (!empty($query_options['sort'])) {
      $elastica_query->setSort($query_options['sort']);
    }

    return $elastica_query;
  }

  /**
   * Helper function return associative array with query options.
   */
  protected function getSearchQueryOptions(SearchApiQueryInterface $query) {

    // Query options.
    $query_options = $query->getOptions();

    //Index fields
    $index_fields = $this->getIndexFields($query);

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
        $query_search_string = new Elastica\Query\QueryString($search_string);
        $query_search_string->setFields(array_values($query_full_text_fields));
      }
    }

    // Sort.
    try {
      $sort = $this->getSortSearchQuery($query);
    }
    catch (Exception $e) {
      watchdog('Elastic Search', check_plain($e->getMessage()), array(), WATCHDOG_ERROR);
      drupal_set_message($e->getMessage(), 'error');
    }

    // Filters.
    $parsed_query_filters = $this->parseFilter($query->getFilter(), $index_fields);
    if (!empty($parsed_query_filters)) {
      $query_search_filter = $parsed_query_filters[0];
    }

    return array(
      'query_offset' => $query_offset,
      'query_limit' => $query_limit,
      'query_search_string' => $query_search_string,
      'query_search_filter' => $query_search_filter,
      'sort' => $sort,
    );
  }

  /**
   * Helper function that return Sort for query in search.
   */
  protected function getSortSearchQuery(SearchApiQueryInterface $query) {

    $index_fields = $this->getIndexFields($query);
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
        throw new Exception(t('Incorrect sorting!.'));
      }
    }
    return $sort;
  }

  /**
   * Helper function build facets in search.
   */
  protected function addSearchFacets(Elastica\Query $elastica_query, SearchApiQueryInterface $query) {

    // SEARCH API FACETS.
    $facets = $query->getOption('search_api_facets');
    $index_fields = $this->getIndexFields($query);

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
          // Add facet options
          $facet = $this->addFacetOptions($facet, $query, $facet_info, $elastica_query);
          $elastica_query->addFacet($facet);
        }
      }
    }
  }

  /**
   * Helper function that add options and return facet
   */
  protected function addFacetOptions(&$facet, SearchApiQueryInterface $query, $facet_info, Elastica\Query $elastica_query) {

    $facet_limit = $this->getFacetLimit($facet_info);
    $facet_search_filter = $this->getFacetSearchFilter($query, $facet_info);
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

    return $facet;
  }

  /**
   * Helper function return Facet filter.
   */
  protected function getFacetSearchFilter(SearchApiQueryInterface $query, $facet_info ) {
    $index_fields = $this->getIndexFields($query);
    $facet_search_filter = '';

    if (isset($facet_info['operator']) && drupal_strtolower($facet_info['operator']) == 'or') {
      $facet_search_filter = $this->parseFilter($query->getFilter(), $index_fields, $facet_info['field']);
      if (!empty($facet_search_filter)) {
        $facet_search_filter = $facet_search_filter[0];
      }
    }
    // Normal facet, we just use the main query filters.
    else {
      $facet_search_filter = $this->parseFilter($query->getFilter(), $index_fields);
      if (!empty($facet_search_filter)) {
        $facet_search_filter = $facet_search_filter[0];
      }
    }

    return $facet_search_filter;
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

  /**
   * Helper function that return facet limits
   */
  protected function getFacetLimit(array $facet_info) {

// If no limit (-1) is selected, use the server facet limit option.
    $facet_limit = !empty($facet_info['limit']) ? $facet_info['limit'] : -1;
    if ($facet_limit < 0) {
      $facet_limit = $this->getOption('facet_limit', 10);
    }
    return $facet_limit;
  }

  /**
   * Helper function which add params to date facets.
   */
  protected function getDateFacetInterval($facet_id) {
    // Active search corresponding to this index.
    $searcher = key(facetapi_get_active_searchers());

    // Get the FacetApiAdpater for this searcher.
    $adapter = isset($searcher) ? facetapi_adapter_load($searcher) : NULL;

    // Get the date granularity.
    $date_gap = $this->getDateGranularity($adapter, $facet_id);

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

    return $date_interval;
  }

  /**
   * Helper function to return date gap.
   */
  public function getDateGranularity($adapter, $facet_id) {
    // Date gaps.
    $gap_weight = array('YEAR' => 2, 'MONTH' => 1, 'DAY' => 0);
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

    return $date_gap;
  }

  /**
   * Helper function which parse facets in search().
   */
  public function parseSearchResponse($response, SearchApiQueryInterface $query) {

    $search_result = array();

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
    $search_result['search_api_facets'] = $this->parseSearchFacets($response, $query);

    return $search_result;
  }

  /**
   *  Helper function that parse facets.
   */
  protected function parseSearchFacets($response, SearchApiQueryInterface $query) {

    $result = array();
    $index_fields = $this->getIndexFields($query);
    $facets = $query->getOption('search_api_facets');

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
                $result[$facet_id][] = array(
                  'count' => $entry['count'],
                  'filter' => '"' . ($entry['time'] / 1000) . '"',
                );
              }
            }
          }
          else {
            foreach ($facet_data['terms'] as $term) {
              if ($term['count'] >= $facet_min_count) {
                $result[$facet_id][] = array(
                  'count' => $term['count'],
                  'filter' => '"' . $term['term'] . '"',
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
   * Helper function. Return the path in the correct format.
   */
  public function setPath($path) {
    if (isset($path) && !empty($path)) {
      $trimmed_path = trim($path, '/');
      $path = $trimmed_path . '/';
    }

    return $path;
  }

  /**
   * Helper function. Get Autocomplete suggestions.
   *
   * @param SearchApiQueryInterface $query
   * @param SearchApiAutocompleteSearch $search
   * @param string $incomplete_key
   * @param string $user_input
   */
  public function getAutocompleteSuggestions(SearchApiQueryInterface $query, SearchApiAutocompleteSearch $search, $incomplete_key, $user_input) {
    $suggestions = array();
    // Turn inputs to lower case, otherwise we get case sensivity problems.
    $incomp = drupal_strtolower($incomplete_key);

    $index = $query->getIndex();
    $index_fields = $this->getIndexFields($query);

    $complete = $query->getOriginalKeys();
    $query->keys($user_input);

    try {
      $response = $this->search($query);
    }
    catch (Exception $e) {
      watchdog('Elastic Search', check_plain($e->getMessage()), array(), WATCHDOG_ERROR);
      return array();
    }

    $matches = array();
    if (isset($response['results'])) {
      $items = $index->loadItems(array_keys($response['results']));
      foreach ($items as $id => $item) {
        $node_title = $index->datasource()->getItemLabel($item);
        $matches[$node_title] = $node_title;
      }

      if ($matches) {
        // Eliminate suggestions that are too short or already in the query.
        foreach ($matches as $name => $node_title) {
          if (drupal_strlen($name) < 3 || isset($keys_array[$name])) {
            unset($matches[$name]);
          }
        }

        // The $count in this array is actually a score. We want the
        // highest ones first.
        arsort($matches);

        // Shorten the array to the right ones.
        $additional_matches = array_slice($matches, $limit - count($suggestions), NULL, TRUE);
        $matches = array_slice($matches, 0, $limit, TRUE);

        foreach ($matches as $node => $name) {
          $suggestions[] = $name;
        }
      }
      $keys = trim($keys . ' ' . $incomplete_key);
      return $suggestions;
    }
  }

  /**
   * Update settings.
   */
  public function updateSettings(SearchApiIndex $index, $data) {
    try {
      $elastica_index = $this->getElasticaIndex($index);
      $elastica_index->getSettings()->set($data);
    }
    catch (Exception $e) {
      watchdog('Elastic Search', check_plain($e->getMessage()), array(), WATCHDOG_ERROR);
      drupal_set_message(check_plain($e->getMessage()), 'error');
      return FALSE;
    }
  }

  /**
   * Get settings.
   */
  public function getSettings(SearchApiIndex $index) {
    try {
      $elastica_index = $this->getElasticaIndex($index);

      if ($elastica_index) {
        $settings = $elastica_index->getSettings()->get();
        if (!empty($settings)) {
          $settings = $this->filterSettings($settings);

          return $settings;
        }
      }
    }
    catch (Exception $e) {
      watchdog('Elastic Search', check_plain($e->getMessage()), array(), WATCHDOG_ERROR);
      drupal_set_message(check_plain($e->getMessage()), 'error');
    }

    return FALSE;
  }

  /**
   * Get settings.
   */
  public function filterSettings($settings) {
    $new_settings = array();

    foreach ($settings as $setting => $value) {
      $filtered_setting = str_replace('index.', '', $setting);
      $new_settings[$filtered_setting] = $value;
    }

    foreach ($new_settings as $new_setting => $new_settings_value) {
      if (in_array($new_setting, array('number_of_shards', 'number_of_replicas'))) {
        unset($new_settings[$new_setting]);
      }
    }

    return $new_settings;
  }

}
