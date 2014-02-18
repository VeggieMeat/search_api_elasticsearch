Elasticsearch backend for the Search API module
===============================================

Requirements
------------

- Drupal 7
- Search API module (http://drupal.org/project/search_api)
- Elasticsearch 1.0.0.0+ (http://www.elasticsearch.org)
- Elastica (elasticsearch PHP Client) (https://github.com/ruflin/Elastica)


Supported features
------------------

- Search API facets
- Search API facets OR
- More Like This

Install Notes
-------------

- See INSTALL.txt

Notes
-----

Elasticsearch handles much of what the Search API preprocessors handle, so in
most cases you should leave Search API preprocessors disabled.

TODO
----
- Add additional search API features support.
