core = 7.x
api = 2

projects[drupal][type] = core
projects[drupal][version] = 7.26
projects[] = composer_manager
projects[] = facetapi
projects[] = search_api
projects[search_api_elasticsearch][type] = "module"
projects[search_api_elasticsearch][download][type] = "git"
projects[search_api_elasticsearch][download][url] = "git://github.com/VeggieMeat/search_api_elasticsearch.git"
