<?php

/**
 * @file
 * Contains SearchApiElasticsearchViewsHandlerFieldDate
 */

class SearchApiElasticsearchViewsHandlerFieldDate extends entity_views_handler_field_date {

  /**
   * @inheritdoc
   */
  public function get_value($values, $field = NULL) {
    foreach ($values->_entity_properties[$this->real_field] as &$value) {
      $value = strtotime($value);
    }
    return EntityFieldHandlerHelper::get_value($this, $values, $field);
  }

}