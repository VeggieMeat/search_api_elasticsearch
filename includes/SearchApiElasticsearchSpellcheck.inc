<?php

/**
 * @file
 * Contains SearchApiElasticsearchSpellcheck
 */

class SearchApiElasticsearchSpellcheck extends SearchApiSpellcheck {
  public function __construct($data) {
    foreach ($data['suggestions'] as $suggestion) {
      $this->addSuggestion(new SearchApiSpellcheckSuggestion($data['text'], $suggestion));
    }
  }
}
