<?php

/**
 * @file
 * Contains searcher for Elastica client.
 */

class SearchApiElasticsearchElasticaSearcher {

  static public function search($type, $query, $options) {
    if (!empty($options['mlt'])) {
      return SearchApiElasticsearchElasticaMLT::search($type, $options, $query);
    }
    return $type->search($query);
  }

}
