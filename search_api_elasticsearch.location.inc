<?php
/**
 * @file
 * provides location_cck capabilities for elasticsearch - search_api integration.
 */

/**
 * Implements hook_field_info_alter().
 */
function search_api_elasticsearch_field_info_alter(&$field_info) {
  if (module_exists('location_cck')) {
    $field_info['location']['label'] = t('Location', array(), array('context' => 'geolocation'));
    $field_info['location']['description'] = t('Store a location.module location.');
    $field_info['location']['settings'] = array();
    $field_info['location']['instance_settings'] = array();
    $field_info['location']['default_widget'] = 'location';
    $field_info['location']['default_formatter'] = 'location_default';
    $field_info['location']['property_type'] = 'location_cck';
    $field_info['location']['property_callbacks'] = array('search_api_elasticsearch_property_info_callback');
    $field_info['location']['microdata'] = TRUE;
  }
}

/**
 * Callback to alter the property info.
 *
 * @see hook_property_info_callbac().
 */
function search_api_elasticsearch_property_info_callback(&$info, $entity_type, $field, $instance, $field_type) {
  $name = $field['field_name'];
  $property = &$info[$entity_type]['bundles'][$instance['bundle']]['properties'][$name];
  $property['type'] = ($field['cardinality'] != 1) ? 'list<geo_point>' : 'geo_point';
  $property['getter callback'] = 'search_api_elasticsearch_field_verbatim_get';
  $property['setter callback'] = 'search_api_elasticsearch_field_verbatim_set';
  $property['property info'] = search_api_elasticsearch_data_property_info('location_cck');
  unset($property['query callback']);
}


/**
 * Return the possible subfields for a location field.
 * //TODO take these from location module.
 *
 * @return array
 */
function _search_api_elasticsearch_location_fields() {
  return array(
    'name' => t('Location name'),
    'street' => t('Street location'),
    'additional' => t('Additional'),
    'city' => t('City'),
    'province' => t('State/Province'),
    'postal_code' => t('Postal code'),
    'country' => t('Country'),
    'locpick' => t('Coordinate Chooser')
  );
}

/**
 * Defines info for the properties of the location field data structure.
 */
function search_api_elasticsearch_data_property_info($name = NULL) {
  $subfields = _search_api_elasticsearch_location_fields();
  $properties = array(
    'latlon' => array(
      'label' => '[EL] LatLong Pair',
      'type' => 'string',
      'getter callback' => 'search_api_elasticsearch_return_latlon_pair',
      'microdata' => FALSE,
    ),
  );

  //For every subfield of the location, add an indexable field.
  foreach ($subfields as $key => $subfield) {
    $properties[$key] = array(
      'label' => $subfield,
      'type' => 'string',
      'getter callback' => 'search_api_elasticsearch_return_location_string',
      'microdata' => FALSE,
    );

  }

  foreach ($properties as $key => &$value) {
    $value += array(
      'description' => !empty($name) ? t('!label of field %name', array(
        '!label' => $value['label'],
        '%name' => $name
      )) : '',
    );
  }
  return $properties;
}

/**
 * Returns the a latlong property form a location.
 */
function search_api_elasticsearch_return_latlon_pair($data, array $options, $name) {
  if ((is_array($data) || (is_object($data) && $data instanceof ArrayAccess)) && !is_null($data['latitude']) && !is_null($data['longitude'])) {
    return $data['latitude'] . ',' . $data['longitude'];
  }
  return NULL;
}

/**
 * Returns the string values from a location.
 */
function search_api_elasticsearch_return_location_string($data, array $options, $name) {
  if ((is_array($data) || (is_object($data) && $data instanceof ArrayAccess)) && !is_null($data[$name])) {
    return $data[$name];
  }
  return NULL;
}


/**
 * Writes the passed field items in the object. Useful as field level setter
 * to set the whole data structure at once.
 */
function search_api_elasticsearch_field_verbatim_set($entity, $name, $items, $langcode, $entity_type) {
  $field = field_info_field($name);
  $langcode = entity_metadata_field_get_language($entity_type, $entity, $field, $langcode);
  $value = $field['cardinality'] == 1 ? array($items) : (array) $items;
  // Filter out any items set to NULL.
  $entity->{$name}[$langcode] = array_filter($value);

  // Empty the static field language cache, so the field system picks up any
  // possible new languages.
  drupal_static_reset('field_language');
}



/**
 * //TODO figure out what to do here.
 * Gets the data from field array.
 */
function search_api_elasticsearch_field_verbatim_get($entity, array $options, $name, $entity_type, &$context) {
  $langcode = isset($options['language']) ? $options['language']->language : LANGUAGE_NONE;
  $langcode = entity_metadata_field_get_language($entity_type, $entity, $context['field'], $langcode, TRUE);
//  $loc = array();
//  $loc['lat'] = $entity->{$name}[$langcode][0]['latitude'];
//  $loc['lon'] = $entity->{$name}[$langcode][0]['longitude'];
  return $entity->{$name}[$langcode][0];
  //return $loc;
}
