<?php

/**
 * @file
 * Contains handler for More Like This queries.
 */

class SearchApiElasticsearchElasticaMLT {

  static public function search($type, $options, $query) {
    $mlt_document = $type->getDocument($options['mlt']['id']);
    $mlt_options['min_term_freq'] = $options['mlt']['min_term_freq'];
    $mlt_options['min_doc_freq'] = $options['mlt']['min_doc_freq'];
    $mlt_options['search_from'] = $options['query_offset'];
    $mlt_options['search_size'] = $options['query_limit'];
    return $type->moreLikeThis($mlt_document, $mlt_options, $query);
  }

}
