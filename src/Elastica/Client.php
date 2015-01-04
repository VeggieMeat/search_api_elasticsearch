<?php

namespace Drupal\search_api_elasticsearch\Elastica;

use Elastica\Client as ElasticaClient;
use Elastica\Request;
use Drupal\search_api_elasticsearch\Logger\RequestLogger;

/**
 * Class Client
 * @package Drupal\search_api_elasticsearch\Elastica
 */
class Client extends ElasticaClient {

  /**
   * @param string $path
   * @param string $method
   * @param array $data
   * @param array $query
   * @return \Elastica\Response
     */
  public function request($path, $method = Request::GET, $data = array(), array $query = array()) {
    $response = parent::request($path, $method, $data, $query);
    $this->logQuery($path, $method, $data, $query);

    return $response;
  }

  /**
   * @param string $path
   * @param string $method
   * @param $data
   * @param array $query
     */
  private function logQuery($path, $method, $data, array $query) {
    if ((!$this->_logger) || (!$this->_logger instanceof RequestLogger)) {
      return;
    }

    $connection = $this->getLastRequest()->getConnection();
    $connection_info = array(
      'host' => $connection->getHost(),
      'port' => $connection->getPort(),
      'transport' => $connection->getTransport(),
      'headers' => $connection->hasConfig('headers') ? $connection->getConfig('headers') : array(),
    );

    $this->_logger->logQuery($path, $method, $data, $connection_info, $query);
  }

}
