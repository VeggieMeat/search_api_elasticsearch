<?php

namespace Drupal\search_api_elasticsearch\tests\Elastica;

use Elastica\Request;
use Elastica\Transport\Null as NullTransport;

class ClientTest extends \PHPUnit_Framework_TestCase {
  public function testRequestsAreLogged() {
    $transport = new NullTransport;
    $connection = $this->getMock('Elastica\Connection');
    $connection->expects($this->any())->method('getTransportObject')->will($this->returnValue($transport));
    $connection->expects($this->any())->method('toArray')->will($this->returnValue(array()));

    $logger = $this->getMock('Drupal\search_api_elasticsearch\Logger\RequestLogger');
    $logger->expects($this->once())
           ->method('logQuery')
           ->with(
             'foo',
             Request::GET,
             $this->isType('array'),
             $this->isType('array'),
             $this->isType('array')
           );
    
    $client = $this->getMockBuilder('Drupal\search_api_elasticsearch\Elastica\Client')
                   ->setConstructorArgs(array(array()))
                   ->setMethods(array('getConnection'))
                   ->getMock();

    $client->expects($this->any())->method('getConnection')->will($this->returnValue($connection));

    $client->setLogger($logger);

    $response = $client->request('foo');

    $this->assertInstanceOf('Elastica\Response', $response);
  }
}
