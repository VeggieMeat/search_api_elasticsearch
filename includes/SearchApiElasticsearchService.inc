<?php
use Elastica\Client;
use Elastica\Document;

/**
 * @file
 * Provides Elasticsearch service for Search API.
 */

class SearchApiElasticsearchService extends SearchApiAbstractService {

  /**
   * @var Client
   */
  protected $_client;

  /**
   * @inheritdoc
   */
  public function indexItems(SearchApiIndex $index, array $items) {
    $elasticsearch_index = new SearchApiElasticsearchIndex($index, $this);

    $documents = [];
    foreach ($items as $id => $fields) {
      $data = array('id' => $id);
      foreach ($fields as $field_id => $field_data) {
        if (isset($field_data['value']) && is_array($field_data['value'])) {
          $data[$field_id] = array();
          foreach ($field_data['value'] as $token) {
            $value = is_array($token) && isset($token['value']) ? $token['value'] : $token;

            if ($field_data['type'] === 'date') {
              $value = date('Y-m-d\TH:i:s.uP', $value);
            }

            $data[$field_id][] = $value;
          }
        }
        else {
          $value = $field_data['value'];

          if ($field_data['type'] === 'date') {
            $value = date('Y-m-d\TH:i:s.uP', $value);
          }

          $data[$field_id][] = $value;
        }
      }

      $documents[] = new Document($id, $data, $index->machine_name);
    }

    return $elasticsearch_index->indexItems($documents);
  }

  /**
   * @inheritdoc
   */
  public function deleteItems($ids = 'all', SearchApiIndex $index = NULL) {
    if (empty($index)) {
      foreach (search_api_index_load_multiple(FALSE, array($this->server->machine_name)) as $index) {
        $this->deleteItems('all', $index);
      }
    }
    elseif ($ids === 'all') {
      $elasticsearch_index = new SearchApiElasticsearchIndex($index, $this);
      $elasticsearch_index->deleteAllItems();
    }
    else {
      $elasticsearch_index = new SearchApiElasticsearchIndex($index, $this);
      $elasticsearch_index->deleteItems($ids);
    }
  }

  /**
   * @inheritdoc
   */
  public function search(SearchApiQueryInterface $query) {
    $elasticsearch_query = new SearchApiElasticsearchQuery($query, $this);
    return $elasticsearch_query->search();
  }

  /**
   * @inheritdoc
   */
  public function configurationForm(array $form, array &$form_state) {
    $defaults = [
        'host' => '127.0.0.1',
        'port' => 9200,
        'path' => '',
        'url' => NULL,
        'transport' => 'Http',
        'persistent' => TRUE,
        'timeout' => 300,
        'log' => FALSE,
        'retryOnConflict' => 0,
      ];
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
      $delta = count($form_state['values']['options']['form']) + 1;
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
            '%default' => $defaults['host'],
          )),
          '#required' => TRUE,
          '#default_value' => isset($this->options[$c]['host']) ? $this->options[$c]['host'] : $defaults['host'],
          '#parents' => array('options', 'form', $c, 'host'),
        );
        // Elasticsearch daemon port.
        $form['daemon_settings']['fieldset'][$c]['port'] = array(
          '#type' => 'textfield',
          '#title' => t('Port'),
          '#description' => t('Port to which the elasticsearch daemon will listen for this server. Default is %default.', array(
            '%default' => $defaults['port'],
          )),
          '#required' => TRUE,
          '#default_value' => isset($this->options[$c]['port']) ? $this->options[$c]['port'] : $defaults['port'],
          '#parents' => array('options', 'form', $c, 'port'),
        );
        //Elasticsearch basic authentication.
        $form['daemon_settings']['fieldset'][$c]['headers'] = array(
          '#type' => 'fieldset',
          '#tree' => TRUE,
          '#title' => t('HTTP Basic Authentication'),
          '#description' => t('If your Elasticsearch server is protected by basic HTTP authentication, enter the login data here.'),
        );
        //Elasticsearch basic authentication username.
        $form['daemon_settings']['fieldset'][$c]['headers']['http_user'] = array(
          '#type' => 'textfield',
          '#title' => t('Username'),
          '#default_value' => (!empty($this->options[$c]['headers']) && !empty($this->options[$c]['headers']['http_user'])) ? $this->options[$c]['headers']['http_user'] : '',
          '#parents' => array('options', 'form', $c, 'headers', 'http_user'),
        );
        //Elasticsearch basic password.
        $form['daemon_settings']['fieldset'][$c]['headers']['http_pass'] = array(
          '#type' => 'password',
          '#title' => t('Password'),
          '#parents' => array('options', 'form', $c, 'headers', 'http_pass'),
        );
        // Elasticsearch daemon path.
        $form['daemon_settings']['fieldset'][$c]['path'] = array(
          '#type' => 'textfield',
          '#title' => t('Elasticsearch path prefix'),
          '#description' => t('Normally empty. Use when you have remapped the Elasticsearch server API path.'),
          '#required' => FALSE,
          '#default_value' => isset($this->options[$c]['path']) ? $this->options[$c]['path'] : $defaults['path'],
          '#parents' => array('options', 'form', $c, 'path'),
        );
        // Elasticsearch daemon URL.
        $form['daemon_settings']['fieldset'][$c]['url'] = array(
          '#type' => 'textfield',
          '#title' => t('Elasticsearch url'),
          '#description' => t('Normally empty. Use instead of host/port when you have remapped the Elasticsearch server API url.'),
          '#required' => FALSE,
          '#default_value' => isset($this->options[$c]['url']) ? $this->options[$c]['url'] : $defaults['url'],
          '#parents' => array('options', 'form', $c, 'url'),
        );
        // Elasticsearch daemon transport.
        $form['daemon_settings']['fieldset'][$c]['transport'] = array(
          '#type' => 'select',
          '#title' => t('Select transport'),
          '#description' => t('Transport to connect to this elasticsearch server.'),
          '#options' => $this->getTransportOptions(),
          '#default_value' => isset($this->options[$c]['transport']) ? $this->options[$c]['transport'] : $defaults['transport'],
          '#parents' => array('options', 'form', $c, 'transport'),
        );
        // Elasticsearch daemon persistent.
        $form['daemon_settings']['fieldset'][$c]['persistent'] = array(
          '#type' => 'checkbox',
          '#title' => t('Persistent connection'),
          '#description' => t('Use persistent connection when connecting to this node.'),
          '#default_value' => isset($this->options[$c]['persistent']) ? $this->options[$c]['persistent'] : $defaults['persistent'],
          '#parents' => array('options', 'form', $c, 'persistent'),
        );
        // Elasticsearch daemon timeout.
        $form['daemon_settings']['fieldset'][$c]['timeout'] = array(
          '#type' => 'textfield',
          '#title' => t('Timeout in ms'),
          '#description' => t('Timeout in ms for waiting this elastic server to respond'),
          '#default_value' => isset($this->options[$c]['timeout']) ? $this->options[$c]['timeout'] : $defaults['timeout'],
          '#parents' => array('options', 'form', $c, 'timeout'),
        );
        // Elasticsearch daemon log.
        $form['daemon_settings']['fieldset'][$c]['log'] = array(
          '#type' => 'checkbox',
          '#title' => t('Log'),
          '#description' => t('Log this elasticsearch server queries to the default log.'),
          '#default_value' => isset($this->options[$c]['log']) ? $this->options[$c]['log'] : $defaults['log'],
          '#parents' => array('options', 'form', $c, 'log'),
        );
        if (!class_exists('\Psr\Log\AbstractLogger')) {
          $form['daemon_settings']['fieldset'][$c]['log']['#disabled'] = TRUE;
          $form['daemon_settings']['fieldset'][$c]['log']['#description'] = t('Logging Elasticsearch queries requires the <a href="@psr3_url">PSR-3 logger</a> to be installed and available. It is recommended to install the <a href="@psr3_watchdog_url">PSR-3 Watchdog module</a> or <a href="@monolog_url">Monolog module</a>.', array('@psr_url' => url('https://packagist.org/packages/psr/log'), '@psr3_watchdog_url' => url('https://drupal.org/project/psr3_watchdog'), '@monolog_url' => url('https://www.drupal.org/project/monolog')));
        }
        // Elasticsearch daemon retryOnConflict.
        $form['daemon_settings']['fieldset'][$c]['retryOnConflict'] = array(
          '#type' => 'textfield',
          '#title' => t('retryOnConflict'),
          '#description' => t('Sets the number of retries of a version conflict occurs because the document was updated between getting it and updating it.'),
          '#default_value' => isset($this->options[$c]['retryOnConflict']) ? $this->options[$c]['retryOnConflict'] : $defaults['retryOnConflict'],
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
        '#default_value' => (isset($this->options['facet_limit'])) ? $this->options['facet_limit'] : $default,
        '#parents' => array('options', 'form', 'facet_limit'),
      );
    }
    return $form;
  }

  /**
   * @inheritdoc
   */
  public function configurationFormValidate(array $form, array &$values, array &$form_state) {
    unset($values['add_more']);
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
      // Put http_user and http_password in correct form.
      if (!empty($setting['headers']['http_user'])) {
        // If username matches the old value and password is empty, then use the old Authentication.
        if (empty($setting['headers']['http_pass'])) {
          if ($setting['headers']['http_user'] == $this->options[$i]['headers']['http_user']) {
            $values[$i]['headers']['Authorization'] = $this->options[$i]['headers']['Authorization'];
          }
          // If username does not match and password is empty, then give a validation error.
          else {
            form_set_error('http_pass', t('If you are changing the username, you need to supply the password.'));
            return;
          }
        }
        else {
          $values[$i]['headers']['Authorization'] = 'Basic ' . base64_encode($setting['headers']['http_user'] . ':' . $setting['headers']['http_pass']);
        }
      }
      if (isset($values[$i]['headers']['http_pass'])) {
        unset($values[$i]['headers']['http_pass']);
      }
    }
  }

  /**
   * @inheritdoc
   */
  public function configurationFormSubmit(array $form, array &$values, array &$form_state) {
    $this->options = $values;
  }

  /**
   * Helper method for accessing Elastica\Client.
   * @return Client
   */
  public function getClient() {
    return ($this->_client) ?: $this->_client = new Client($this->configureClient());
  }

  /**
   * Configure Elastica\Client.
   *
   * @return array
   */
  protected function configureClient() {
    $config = [];

    if (count($this->options) > 1) {
      foreach ($this->options as $id => $option) {
        $config[] = $option;
      }
    }
    else {
      $config = $this->options;
    }

    return [
      'servers' => $config,
    ];
  }

  /**
   * Get available transport options.
   *
   * @return array
   *   An array of available transport options.
   *   Key: Transport name used by Elastica.
   *   Value: Display name.
   */
  protected function getTransportOptions() {
    $options = [
      'Http' => 'HTTP',
      'Https' => 'HTTPS',
      'Null' => 'NULL',
    ];

    if (class_exists('\Guzzle\Client')) {
      $options['Guzzle'] = 'Guzzle';
    }

    if (class_exists('\Aws\AwsClient')) {
      $options['AwsAuthV4'] = 'AwsAuthV4';
    }

    return $options;
  }

  /**
   * Get information for display on Search API Server page.
   * @return array
   */
  public function getExtraInformation() {
    return [
      [
        'label' => t('Elasticsearch Version'),
        'info' => $this->getClient()->getVersion(),
      ],
    ];
  }

  /**
   * @inheritdoc
   */
  public function supportsFeature($feature) {
    $supported = drupal_map_assoc([
      'search_api_facets',
      'search_api_facets_operator_or',
      'search_api_service_extra'
    ]);

    return isset($supported[$feature]);
  }

  /**
   * @inheritdoc
   */
  public function addIndex(SearchApiIndex $index) {
    $new_index = new SearchApiElasticsearchIndex($index, $this);
    $new_index->create();
    $new_index->addAlias();
  }

  /**
   * @inheritdoc
   */
  public function removeIndex($index) {
    $deleted_index = new SearchApiElasticsearchIndex($index, $this);
    $deleted_index->delete();
  }

  /**
   * @inheritdoc
   */
  public function fieldsUpdated(SearchApiIndex $index) {
    $helper = new SearchApiElasticsearchIndex($index, $this);
    return $helper->updateFields();
  }

  /**
   * Helper function. Return the path in the correct format.
   *
   * @param string $path
   *
   * @return string
   */
  protected function setPath($path) {
    if (isset($path) && !empty($path)) {
      $trimmed_path = trim($path, '/');
      $path = $trimmed_path . '/';
    }

    return $path;
  }

}