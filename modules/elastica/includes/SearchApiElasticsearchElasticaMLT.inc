<?php

/**
 * @file
 * Contains handler for More Like This queries.
 */

class SearchApiElasticsearchElasticaMLT {

  static public function build($type, $options, $query, SearchApiQueryInterface $search_api_query) {
    self::filterFields($options['mlt']['fields'], $search_api_query);
    $mlt_query = new Elastica\Query\MoreLikeThis();
    $mlt_query->setFields(array_values($options['mlt']['fields']));
    $mlt_query->setParam('ids', array($options['mlt']['id']));
    $mlt_query->setMinDocFrequency($options['mlt']['min_doc_freq']);
    $mlt_query->setMinTermFrequency($options['mlt']['min_term_freq']);
    $query->setQuery($mlt_query);

    if (!empty($options['query_search_filter'])) {
      $query->setQuery(new Elastica\Query\Filtered($mlt_query, $options['query_search_filter']));
    }
  }

  /**
   * More Like This can only use text type fields.
   *
   * @param mixed $fields
   * @access private
   * @return void
   */
  private static function filterFields(&$fields, SearchApiQueryInterface $query) {
    $index = $query->getIndex();
    $index_fields = $index->getFields();
    foreach ($index_fields as $name => $field) {
      if (!search_api_is_text_type($field['type'])) {
        $key = array_search($name, $fields);
        if ($key !== FALSE) {
          unset($fields[$key]);
        }
      }
    }
  }

}
