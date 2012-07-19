Elasticsearch backend for the Search API module
===============================================

Requirements
------------

- Drupal 7
- Search API module (http://drupal.org/project/search_api)
- Elasticsearch 0.19.8+ (http://www.elasticsearch.org)
- Elastica (elasticsearch PHP Client) (https://github.com/ruflin/Elastica)


Supported features
------------------

- Search API facets
- Search API facets OR

Install Notes
-------------

- See INSTALL.txt

Notes
-----

- Currently Only one host and port are supported (no cluster).
  It will be changed soon to be in the server options.

- Currently no options to set up the number of shards and
  replicas for the indices. To be changed.

TODO
----

- Error handling.
- Server and index options.
- Improve facet handling.
- Add additional search API features support.
