<?php

class SearchApiElasticsearchAbstractServiceTest extends PHPUnit_Framework_TestCase {

  /**
   * @var SearchApiElasticsearchAbstractServiceTest
   */
  protected $_sut;

  protected function setUp() {
    $this->_sut = $this->getMockForAbstractClass('SearchApiElasticsearchAbstractService');
  }

  protected function tearDown() {
  }

  public function testConfigurationForm() {
    $this->markTestIncomplete('Not implemented');
  }

}
