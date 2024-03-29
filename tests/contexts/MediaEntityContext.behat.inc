<?php

namespace Acquia\LightningExtension\Context;

use Behat\Gherkin\Node\PyStringNode;
use Behat\Gherkin\Node\TableNode;
use Behat\Mink\Exception\UnsupportedDriverActionException;
use Drupal\Component\Utility\Random;
use Drupal\DrupalExtension\Context\DrupalSubContextBase;
use Drupal\file\Entity\File;
use Drupal\media\Entity\Media;
use Drupal\Tests\lightning_media\FixtureContext;

/**
 * Contains step definitions for interacting with media entities.
 */
final class MediaEntityContext extends DrupalSubContextBase {

  use AwaitTrait;

  /**
   * @BeforeScenario
   */
  public function setUp() {
    // Automatically delete all media items created by the user during this
    // scenario.
    /** @var \Drupal\Tests\lightning_media\FixtureContext $fixtureContext */
    if ($fixtureContext = $this->getContext(FixtureContext::class)) {
      $fixtureContext->trackUserContent($this->getUserManager(), 'media');
    }
  }

  /**
   * Creates a set of media items from tabular data.
   *
   * @param \Behat\Gherkin\Node\TableNode $table
   *   The table of entity data.
   *
   * @Given media items:
   */
  public function createMultiple(TableNode $table) {
    $this->getContext(EntityContext::class)->createMultiple('media', $table);
  }

  /**
   * Asserts that we are visiting a media entity.
   *
   * @Then I should be visiting a media item
   */
  public function assertMediaItemPage() {
    $assert = $this->assertSession();

    // The path must start with "/media/" and end with "/ID"; the bundle may be
    // in the middle if Pathauto is installed.
    $assert->addressMatches('#^/media/([\w-]+/)?\d+$#');

    try {
      $assert->statusCodeEquals(200);
    }
    catch (UnsupportedDriverActionException $e) {
      // This isn't a critically important assertion, so don't worry about it.
    }
  }

  /**
   * Creates a media item from an embed code.
   *
   * @param string $bundle
   *   The media bundle ID.
   * @param \Behat\Gherkin\Node\PyStringNode $embed_code
   *   The embed code.
   *
   * @Given :bundle media from embed code:
   *
   * @When I create :bundle media from embed code:
   */
  public function fromEmbedCode($bundle, PyStringNode $embed_code) {
    $media = Media::create([
      'bundle' => $bundle,
      'name' => $this->getRandom()->string(),
      'embed_code' => (string) $embed_code,
      'field_media_in_library' => TRUE,
    ]);
    $this->getContext(EntityContext::class)->save($media->setPublished());
  }

  /**
   * Generates random images, saved as media items.
   *
   * @param int $n
   *   (optional) How many to generate. Defaults to 1.
   * @param string $name
   *   (optional) The label of the media item wrapping the image. Defaults to
   *   a random string.
   * @param string $alt
   *   (optional) The alt text for the image.
   *
   * @Given a random image
   * @Given a random image named :name
   * @Given a random image with alt text :alt
   * @Given a random image named :name with alt text :alt
   * @Given :count random images
   */
  public function randomImage($n = 1, $name = NULL, $alt = NULL) {
    $random = new Random();

    /** @var EntityContext $context */
    $context = $this->getContext(EntityContext::class);

    for ($i = 0; $i < $n; $i++) {
      $uri = $random->image(uniqid('public://random_') . '.png', '240x240', '640x480');

      $file = File::create([
        'uri' => $uri,
      ]);
      $file->setMimeType('image/png');
      $file->setTemporary();
      $file->save();

      $media = Media::create([
        'bundle' => 'image',
        'name' => $name ?: $random->name(32),
        'image' => $file->id(),
        'field_media_in_library' => TRUE,
      ]);
      if ($alt) {
        $media->image->alt = $alt;
      }
      $context->save($media->setPublished());
    }
  }

  /**
   * Generates random text files, saved as media items.
   *
   * @param int $n
   *   (optional) How many to generate. Defaults to 1.
   *
   * @Given a random document
   * @Given :count random documents
   */
  public function randomDocument($n = 1) {
    $random = new Random();

    /** @var EntityContext $context */
    $context = $this->getContext(EntityContext::class);

    for ($i = 0; $i < $n; $i++) {
      $uri = uniqid('public://random_') . '.txt';
      file_put_contents($uri, $random->paragraphs(2));

      $file = File::create([
        'uri' => $uri,
      ]);
      $file->setMimeType('text/plain');
      $file->setTemporary();
      $file->save();

      $media = Media::create([
        'bundle' => 'document',
        'name' => $random->name(32),
        'image' => $file->id(),
        'field_media_in_library' => TRUE,
      ]);
      $context->save($media->setPublished());
    }
  }

  /**
   * Asserts that a slideshow of media assets is present on the page.
   *
   * @Then I should see a slideshow of media assets
   */
  public function assertSlideshow() {
    $assert = $this->assertSession();
    $assert->elementExists('css', 'button.slick-prev.slick-arrow');
    $assert->elementExists('css', 'button.slick-next.slick-arrow');
  }

  /**
   * Asserts an entity embedding form for a non-image media item.
   *
   * @Then I should be able to embed the media item
   */
  public function assertGenericEntityEmbedForm() {
    $assert_session = $this->assertSession();

    $form = $assert_session->elementExists('css', '.entity-embed-dialog');
    $assert_session->fieldNotExists('Image style', $form);
    $assert_session->fieldNotExists('Alternate text', $form);
    $assert_session->fieldNotExists('Title', $form);
  }

  /**
   * Embeds a media item with a set of embedding options.
   *
   * @param \Behat\Gherkin\Node\TableNode $options
   *   The embedding options. Only the first row of the table is used.
   *
   * @When I embed the media item with options:
   */
  public function embedEntity(TableNode $options) {
    $assert_session = $this->assertSession();

    $options = $options->getHash();

    $form = $assert_session->elementExists('css', '.entity-embed-dialog');
    foreach (current($options) as $field => $value) {
      $assert_session->fieldExists($field, $form)->setValue($value);
    }

    $assert_session->elementExists('css', 'body > .ui-dialog .ui-dialog-buttonpane button.button--primary')
      ->click();
    $this->awaitAjax();
  }

}
