<?php

/**
 * @file
 * Contains SearchApiElasticsearchViewsHandlerFilterDate
 */

class SearchApiElasticsearchViewsHandlerFilterDate extends SearchApiViewsHandlerFilterDate {

  /**
   * @inheritdoc
   */
  public function query() {
    if ($this->operator === 'empty') {
      $this->query->condition($this->real_field, NULL, '=', $this->options['group']);
    }
    elseif ($this->operator === 'not empty') {
      $this->query->condition($this->real_field, NULL, '<>', $this->options['group']);
    }
    else {
      while (is_array($this->value)) {
        $this->value = $this->value ? reset($this->value) : NULL;
      }
      $v = is_numeric($this->value) ? $this->value : strtotime($this->value, REQUEST_TIME);
      $query = new EntityFieldQuery();
      $query->entityCondition('entity_type', 'search_api_index', '=');
      $query->propertyCondition('machine_name', str_replace('search_api_index_', '', $this->view->base_table), '=');
      $result = $query->execute();
      $result = reset($result['search_api_index']);
      $index = search_api_index_load($result->id);
      $server = $index->server();
      $class = search_api_get_service_info($server->class);
      if (is_numeric($v) && $class['class'] === 'SearchApiElasticsearchService') {
        $v = date('Y-m-d\TH:i:s.uP', $v);
      }
      if ($v !== FALSE) {
        $this->query->condition($this->real_field, $v, $this->operator, $this->options['group']);
      }
    }
  }

}