<?php

/**
 * @file
 * Contains searcher for Elastica client.
 */

class SearchApiElasticsearchElasticaSearcher {

  static public function search($type, $query, $options) {
    if (!empty($options['mlt'])) {
      SearchApiElasticsearchElasticaMLT::build($type, $options, $query);
    }
    return $type->search($query);
  }

}
