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

    $analyzers = $this->getAnalyzers();
    $form['analyzers'] = array(
      '#title' => t('Analyzers'),
      '#type' => 'fieldset',
      '#description' => t('Select the analyzers to use on this search. If none are selected, the default will be used. See <a href="@elasticsearch_analyzers_url">Elasticsearch Analyzers</a>.', array('@elasticsearch_analyzers_url' => url('http://www.elasticsearch.org/guide/en/elasticsearch/reference/current/analysis-analyzers.html'))),
      '#collapsible' => FALSE,
      '#tree' => TRUE,
    );

    $form['analyzers']['standard'] = array(
      '#title' =>t('Standard'),
      '#type' => 'fieldset',
      '#description' => t('An analyzer built using the Standard Tokenizer with the Standard Token Filter, Lower Case Token Filter, and Stop Token Filter.'),
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
      '#tree' => TRUE,
    );

    $form['analyzers']['standard']['enabled'] = array(
      '#title' => t('Enabled'),
      '#type' => '#checkbox',
    );

    $form['analyzers']['standard']['stopwords'] = array(
      '#title' => t('Stopwords'),
      '#type' => 'textarea',
      '#description' => t('A list of stopwords to initialize the stop filter with. Defaults to an empty stopword list.'),
    );

    $form['analyzers']['standard']['max_token_length'] = array(
      '#title' => t('Maximum Token Length'),
      '#type' => 'textfield',
      '#description' => t('The maximum token length. If a token is seen that exceeds this length then it is discarded. Defaults to 255.'),
    );

    $form['analyzers']['simple'] = array(
      '#title' =>t('Simple'),
      '#type' => 'fieldset',
      '#description' => t('An analyzer built using a Lower Case Tokenizer.'),
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
      '#tree' => TRUE,
    );

    $form['analyzers']['simple']['enabled'] = array(
      '#title' => t('Enabled'),
      '#type' => '#checkbox',
    );

    $form['analyzers']['whitespace'] = array(
      '#title' =>t('Whitespace'),
      '#type' => 'fieldset',
      '#description' => t('An analyzer built using a Whitespace Tokenizer.'),
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
      '#tree' => TRUE,
    );

    $form['analyzers']['whitespace']['enabled'] = array(
      '#title' => t('Enabled'),
      '#type' => '#checkbox',
    );

    $form['analyzers']['stop'] = array(
      '#title' =>t('Stop'),
      '#type' => 'fieldset',
      '#description' => t('An analyzer built using a Lower Case Tokenizer, with Stop Token Filter.'),
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
      '#tree' => TRUE,
    );

    $form['analyzers']['stop']['enabled'] = array(
      '#title' => t('Enabled'),
      '#type' => '#checkbox',
    );

    $form['analyzers']['stop']['stopwords'] = array(
      '#title' => t('Stopwords'),
      '#type' => 'textarea',
      '#description' => t('A list of stopwords to initialize the stop filter with. Defaults to the English stopwords.'),
    );

    $form['analyzers']['keyword'] = array(
      '#title' =>t('Keyword'),
      '#type' => 'fieldset',
      '#description' => t('An analyzer that "tokenizes" an entire stream as a single token. This is useful for data like zip codes, ids and so on.'),
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
      '#tree' => TRUE,
    );

    $form['analyzers']['keyword']['enabled'] = array(
      '#title' => t('Enabled'),
      '#type' => '#checkbox',
    );

    $form['analyzers']['pattern'] = array(
      '#title' =>t('Pattern'),
      '#type' => 'fieldset',
      '#description' => t('An analyzer that can flexibly separate text into terms via a regular expression.'),
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
      '#tree' => TRUE,
    );

    $form['analyzers']['pattern']['enabled'] = array(
      '#title' => t('Enabled'),
      '#type' => '#checkbox',
    );

    $form['analyzers']['pattern']['lowercase'] = array(
      '#title' => t('Lowercase'),
      '#type' => 'checkbox',
    );

    $form['analyzers']['pattern']['pattern'] = array(
      '#title' => t('Pattern'),
      '#type' => 'textarea',
      '#description' => t('The regular expression pattern. The regular expression should match the token separators, not the tokens themselves. Defaults to \W+.'),
    );

    $form['analyzers']['pattern']['flags'] = array(
      '#title' => t('Flags'),
      '#type' => 'textfield',
      '#description' => t('The regular expression flags. Flags should be pipe-separated, eg "CASE_INSENSITIVE|COMMENTS". Check Java Pattern API for more details about flags options.'),
    );

    $form['analyzers']['pattern']['stopwords'] = array(
      '#title' => t('Stopwords'),
      '#type' => 'textarea',
      '#description' => t('A list of stopwords to initialize the stop filter with. Defaults to an empty stopwords list.'),
    );

    // @TODO Determine proper way to implement language analyzers.
    /*$form['analyzers']['language'] = array(
      '#title' =>t('Language'),
      '#type' => 'fieldset',
      '#description' => t('An analyzer that can flexibly separate text into terms via a regular expression.'),
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
      '#tree' => TRUE,
    );*/

    $form['analyzers']['snowball'] = array(
      '#title' =>t('Snowball'),
      '#type' => 'fieldset',
      '#description' => t('An analyzer that uses the standard tokenizer, with standard filter, lowercase filter, stop filter, and snowball filter.'),
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
      '#tree' => TRUE,
    );

    $form['analyzers']['snowball']['enabled'] = array(
      '#title' => t('Enabled'),
      '#type' => '#checkbox',
    );

    // @TODO Determine proper way to implement custom analyzers.
    /*$form['analyzers']['custom'] = array(
      '#title' =>t('Custom'),
      '#type' => 'fieldset',
      '#description' => t('An analyzer that allows to combine a Tokenizer with zero or more Token Filters, and zero or more Char Filters.'),
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
      '#tree' => TRUE,
    );*/
  }
}
