<?php
/**
 * @file
 *
 * Drush integration for Search API elasticsearch.
 *
 * Part of the code is borrowed from Joseph McGuire (JoeMcGuire)'s
 * elasticsearch module.
 */

/**
 * Implements hook_drush_command().
 */
function search_api_elasticsearch_drush_command() {
  $items['download-elastica'] = array(
    'description' => dt("Downloads the Elastica PHP Library."),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_FULL,
    'callback' => 'drush_download_elastica',
  );
  return $items;
}

/**
 * Command to download the Elastica PHP Library.
 */
function drush_download_elastica() {
  module_load_include('module', 'search_api_elasticsearch');
  spl_autoload_register('_search_api_elasticsearch_autoload');

  // Check if the library is already installed.
  try {
    class_exists('Elastica\Client');
    drush_log(dt('Elastica library already present. No download required.'), 'ok');
  }
  catch (Exception $exception) {
    $path = DRUPAL_ROOT . '/sites/all/libraries';

    // Create the libraries path if it does not exist.
    if (!is_dir($path)) {
      drush_op('mkdir', $path);

      if (is_dir($path)) {
        drush_log(dt('Libraries directory @path was created', array('@path' => $path)), 'notice');
      }
      else {
        drush_log(dt('Drush was unable to create the libraries directory @path.', array('@path' => $path)), 'error');
        return;
      }
    }

    // Clone the Elastica library.
    if (drush_shell_cd_and_exec($path, 'git clone https://github.com/ruflin/Elastica.git')) {
      drush_log(dt('Elastica has been cloned via git to @path.', array('@path' => $path)), 'success');

      // Verify installation.
      if (is_dir(_search_api_elasticsearch_elastica_path(TRUE))) {
        drush_log(dt('Elastica directory present.'), 'ok');
      }
      if (class_exists('Elastica\Client')) {
        drush_log(dt('Elastica library has been properly installed.'), 'ok');
      }
    }
    else {
      drush_log(dt('Drush was unable to clone Elastica to @path.', array('@path' => $path)), 'error');
    }
  }
}

/**
 * Implements drush_MODULE_pre_COMMAND().
 *
 * Attempt to download the Elastica library if not already there.
 */
function drush_search_api_elasticsearch_pre_pm_enable() {
  $modules = func_get_args();
  if (in_array('search_api_elasticsearch', $modules)) {
    drush_download_elastica();
  }
}
