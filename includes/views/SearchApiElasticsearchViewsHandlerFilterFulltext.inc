<?php

/**
 * @file
 * Contains SearchApiElasticsearchViewsHandlerFilterFulltext.
 */

/**
 * Views filter handler class for handling fulltext fields.
 */

class SearchApiElasticsearchViewsHandlerFilterFulltext extends SearchApiViewsHandlerFilterFulltext {

  /**
   * Extend the options form.
   */
  public function options_form(&$form, &$form_state) {
    parent::options_form($form, $form_state);
  }
}
