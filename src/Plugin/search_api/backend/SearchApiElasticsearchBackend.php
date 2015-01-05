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
use Drupal\search_api_elasticsearch\Elastica\Client;
use Elastica\Document;
use Elastica\Query\MatchAll;

/**
 * @SearchApiBackend(
 *   id = "search_api_elasticsearch",
 *   label = @Translation("Elasticsearch"),
 *   description = @Translation("Use Elasticsearch as a Search API backend.")
 * )
 */
class SearchApiElasticsearchBackend extends BackendPluginBase {

  /**
   * @var \Drupal\Core\Form\FormBuilderInterface
   */
  protected $formBuilder;

  /**
   * @var \Drupal\Core\Extension\ModuleHandlerInterface
   */
  protected $moduleHandler;

  /**
   * @var \Drupal\Core\Config\Config
   */
  protected $searchApiElasticsearchSettings;

  /**
   * @var \Drupal\search_api_elasticsearch\Elastica\Client
   */
  protected $client;

  /**
   * @var array
   */
  protected $_fieldDataForIndexing = [];

  /**
   * {@inheritdoc}
   */
  public function __construct(array $configuration, $plugin_id, array $plugin_definition, FormBuilderInterface $form_builder, ModuleHandlerInterface $module_handler, Config $settings) {
    parent::__construct($configuration, $plugin_id, $plugin_definition);

    $this->formBuilder = $form_builder;
    $this->moduleHandler = $module_handler;
    $this->searchApiElasticsearchSettings = $settings;
    $this->client = new Client($configuration);
  }

  /**
   * Indexes the specified items.
   *
   * @param \Drupal\search_api\IndexInterface $index
   *   The search index for which items should be indexed.
   * @param \Drupal\search_api\Item\ItemInterface[] $items
   *   An array of items to be indexed, keyed by their item IDs.
   *   The value of fields with the "tokenized_text" type is an array of tokens.
   *   Each token is an array containing the following keys:
   *   - value: The word that the token represents.
   *   - score: A score for the importance of that word.
   *
   * @return array
   *   The IDs of all items that were successfully indexed.
   *
   * @throws \Drupal\search_api\SearchApiException
   *   If indexing was prevented by a fundamental configuration error.
   */
  public function indexItems(IndexInterface $index, array $items) {
    $type = $this->getElasticsearchType($index);

    if (empty($type) || empty($items)) {
      return [];
    }

    $documents = [];
    $return = [];
    foreach ($items as $id => $fields) {
      $this->_fieldDataForIndexing = array('id' => $id);
      $this->parseFieldsForIndexing($fields);

      $documents[] = new Document($id, $data);
      $return[] = $id;
    }

    try {
      $type->addDocuments($documents);
    }
    catch (\Exception $e) {
      // @TODO Implement this - possibly with own Exception handlers
    }

    return $return;
  }

  /**
   * Deletes the specified items from the index.
   *
   * @param \Drupal\search_api\IndexInterface $index
   *   The index from which items should be deleted.
   * @param string[] $item_ids
   *   The IDs of the deleted items.
   *
   * @throws \Drupal\search_api\SearchApiException
   *   If an error occurred while trying to delete the items.
   */
  public function deleteItems(IndexInterface $index, array $item_ids) {
    $type = $this->getElasticsearchType($index);

    if (empty($type)) {
      return;
    }

    $type->deleteIds($item_ids);
  }

  /**
   * Deletes all the items from the index.
   *
   * @param \Drupal\search_api\IndexInterface $index
   *   The index for which items should be deleted.
   *
   * @throws \Drupal\search_api\SearchApiException
   *   If an error occurred while trying to delete the items.
   */
  public function deleteAllIndexItems(IndexInterface $index) {
    $type = $this->getElasticsearchType($index);

    if (empty($type)) {
      return;
    }

    $query = new MatchAll();
    $type->deleteByQuery($query);
  }

  /**
   * Executes a search on this server.
   *
   * @param \Drupal\search_api\Query\QueryInterface $query
   *   The query to execute.
   *
   * @return \Drupal\search_api\Query\ResultSetInterface
   *   The search results.
   *
   * @throws \Drupal\search_api\SearchApiException
   *   If an error prevented the search from completing.
   */
  public function search(QueryInterface $query) {
    // TODO: Implement search() method.
  }

  /**
   * @param \Drupal\search_api\IndexInterface $index
   * @return \Elastica\Type|null
   */
  private function getElasticsearchType(IndexInterface $index) {
    if ($index instanceof IndexInterface) {
      $elasticsearch_index = $this->getElasticsearchIndex($index);
      return !empty($elasticsearch_index) ? $elasticsearch_index->getType($index->machine_name) : NULL;
    }
  }

  /**
   * @param \Drupal\search_api\IndexInterface $index
   * @return \Elastica\Index
   */
  private function getElasticsearchIndex(IndexInterface $index) {
    $name = $this->getIndexName($index);
    try {
      $elasticsearch_index = $this->client->getIndex($name);
      return $elasticsearch_index;
    }
    catch (\Exception $e) {
      // @TODO Implement
    }
  }

  /**
   * @param \Drupal\search_api\IndexInterface $index
   */
  private function getIndexName(IndexInterface $index) {

  }

  /**
   * @param array $fields
   */
  private function parseFieldsForIndexing(array $fields) {
    foreach ($fields as $field_id => $field_data) {
      if (isset($field_data['value']) && is_array($field_data['value'])) {
        $this->parseMultivalueFieldData($field_id, $field_data['value']);
      }

      $this->_fieldDataForIndexing[$field_id] = $field_data['value'];
    }
  }

  /**
   * @param $field_id
   * @param array $values
   */
  private function parseMultivalueFieldData($field_id, array $values) {
    foreach ($values as $value) {
      if (is_scalar($value)) {
        $this->_fieldDataForIndexing[$field_id][] = $value;
      }
      else if (is_array($value) && isset($value['value'])) {
        $this->_fieldDataForIndexing[$field_id][] = $value['value'];
      }
    }
  }
}
