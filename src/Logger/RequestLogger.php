<?php

namespace Drupal\search_api_elasticsearch\Logger;

use Drupal\Core\Logger\RfcLoggerTrait;
use Psr\Log\LoggerInterface;

/**
 * Class RequestLogger
 * @package Drupal\search_api_elasticsearch\Logger
 */
class RequestLogger implements LoggerInterface {
  use RfcLoggerTrait;

  /**
   * @param LoggerInterface $logger
   * @param bool $debug
     */
  public function __construct(LoggerInterface $logger = null, $debug = false) {
    $this->logger = $logger;
    $this->debug = $debug;
  }

  /**
   * @param $path
   * @param $method
   * @param $data
   * @param array $connection
   * @param array $query
     */
  public function logQuery($path, $method, $data, $connection = array(), $query = array()) {
    if ($this->debug) {
      $this->queries[] = array(
        'path' => $path,
        'method' => $method,
        'data' => $data,
        'connection' => $connection,
        'queryString' => $query,
      );
    }

    if ($this->logger !== null) {
      $message = sprintf("%s (%s)", $path, $method);
      $this->logger->info($message, (array) $data);
    }
  }

  /**
   * {@inheritdoc}
   */
  public function log($level, $message, array $context = array()) {
    return $this->logger->log($level, $message, $context);
  }
}
