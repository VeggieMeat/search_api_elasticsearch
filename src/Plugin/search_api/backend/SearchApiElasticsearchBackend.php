<?php

/**
 * @file
 * Contains \Drupal\search_api_elasticsearch\Plugin\search_api\backend\SearchApiElasticsearchBackend.
 */

namespace Drupal\search_api_elasticsearch\Plugin\search_api\backend;

use Drupal\Core\Config\Config;
use Drupal\Core\Extension\ModuleHandlerInterface;
use Drupal\Core\Form\FormBuilderInterface;
use Drupal\Core\Form\FormStateInterface;
use Drupal\search_api\SearchApiException;
use Drupal\search_api\IndexInterface;
use Drupal\search_api\Query\FilterInterface;
use Drupal\search_api\Query\QueryInterface;
use Drupal\search_api\Backend\BackendPluginBase;
use Drupal\search_api\Query\ResultSetInterface;

/**
 * @SearchApiBackend(
 *   id = "search_api_elasticsearch",
 *   label = @Translation("Elasticsearch"),
 *   description = @Translation("Use Elasticsearch as a Search API backend.")
 * )
 */
class SearchApiElasticsearchBackend extends BackendPluginBase {

  /**
   * {@inheritdoc}
   */
  public function __construct(array $configuration, $plugin_id, array $plugin_definition, FormBuilderInterface $form_builder, ModuleHandlerInterface $module_handler, Config $settings) {
    parent::__construct($configuration, $plugin_id, $plugin_definition);
    $this->formBuilder = $form_builder;
    $this->moduleHandler = $module_handler;
    $this->searchApiElasticsearchSettings = $settings;
  }

}
