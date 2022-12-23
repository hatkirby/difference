#include "difference.h"
#include <vector>
#include <iostream>
#include <yaml-cpp/yaml.h>
#include <chrono>
#include <thread>
#include <deque>
#include <json.hpp>
#include "imagenet.h"

#define ENABLE_BOT

difference::difference(
  std::string configFile,
  std::mt19937& rng) :
    rng_(rng)
{
  // Load the config file.
  YAML::Node config = YAML::LoadFile(configFile);

#ifdef ENABLE_BOT
  // Set up the Mastodon client.
  instance_ = std::make_unique<mastodonpp::Instance>(
    config["mastodon_instance"].as<std::string>(),
    config["mastodon_token"].as<std::string>());
  connection_ = std::make_unique<mastodonpp::Connection>(*instance_);
#endif
	
  // Set up the verbly database.
  database_ = std::unique_ptr<verbly::database>(
    new verbly::database(config["verbly_datafile"].as<std::string>()));

  imagenet_ = std::make_unique<imagenet>(config["imagenet"].as<std::string>());

  fontfile_ = "@" + config["font"].as<std::string>();
  tempfile_ = config["tempfile"].as<std::string>();
}

void difference::run() const
{
  for (;;)
  {
    std::cout << "Generating tweet..." << std::endl;

    try
    {
      // Find a noun to use as the pictured item.
      std::cout << "Choosing pictured noun..." << std::endl;

      verbly::word pictured = getPicturedNoun();

      std::cout << "Noun: " << pictured.getBaseForm().getText() << std::endl;

      // Choose two pictures of that noun.
      std::cout << "Finding an image..." << std::endl;

      Magick::Image image1;
      Magick::Image image2;
      std::tie(image1, image2) = getImagesForNoun(pictured);

      // Choose two opposite words.
      std::cout << "Choosing two opposite words..." << std::endl;

      verbly::word word1;
      verbly::word word2;
      std::tie(word1, word2) = getOppositeIdentifiers();

      // Compose the images and words.
      std::cout << "Composing image..." << std::endl;

      Magick::Image image = composeImage(
        std::move(image1),
        word1,
        std::move(image2),
        word2);

      // Generate the tweet text.
      std::cout << "Generating text..." << std::endl;

      std::string text = generatePostText(std::move(word1), std::move(word2));

      std::cout << "Post text: " << text << std::endl;

      // Send the post.
      std::cout << "Sending post..." << std::endl;

      sendPost(std::move(text), std::move(image));

      std::cout << "Posted!" << std::endl;

      // Wait.
      std::this_thread::sleep_for(std::chrono::hours(9));
    } catch (const could_not_get_images& ex)
    {
      std::cout << ex.what() << std::endl;
    } catch (const Magick::ErrorImage& ex)
    {
      std::cout << "Image error: " << ex.what() << std::endl;
    } catch (const Magick::ErrorCorruptImage& ex)
    {
      std::cout << "Corrupt image: " << ex.what() << std::endl;
    } catch (const std::exception& ex)
    {
      std::cout << "Other error: " << ex.what() << std::endl;

      std::this_thread::sleep_for(std::chrono::hours(1));
    }

    std::cout << std::endl;
  }
}

verbly::word difference::getPicturedNoun() const
{
  verbly::filter whitelist =
    (verbly::notion::wnid == 109287968)    // Geological formations
    || (verbly::notion::wnid == 109208496) // Asterisms (collections of stars)
    || (verbly::notion::wnid == 109239740) // Celestial bodies
    || (verbly::notion::wnid == 109277686) // Exterrestrial objects (comets and meteroids)
    || (verbly::notion::wnid == 109403211) // Radiators (supposedly natural radiators but actually these are just pictures of radiators)
    || (verbly::notion::wnid == 109416076) // Rocks
    || (verbly::notion::wnid == 105442131) // Chromosomes
    || (verbly::notion::wnid == 100324978) // Tightrope walking
    || (verbly::notion::wnid == 100326094) // Rock climbing
    || (verbly::notion::wnid == 100433458) // Contact sports
    || (verbly::notion::wnid == 100433802) // Gymnastics
    || (verbly::notion::wnid == 100439826) // Track and field
    || (verbly::notion::wnid == 100440747) // Skiing
    || (verbly::notion::wnid == 100441824) // Water sport
    || (verbly::notion::wnid == 100445351) // Rowing
    || (verbly::notion::wnid == 100446980) // Archery
      // TODO: add more sports
    || (verbly::notion::wnid == 100021939) // Artifacts
    || (verbly::notion::wnid == 101471682) // Vertebrates
      ;

  verbly::filter blacklist =
    (verbly::notion::wnid == 106883725) // swastika
    || (verbly::notion::wnid == 104416901) // tetraskele
    || (verbly::notion::wnid == 102512053) // fish
    || (verbly::notion::wnid == 103575691) // instrument of execution
    || (verbly::notion::wnid == 103829563) // noose
    || (verbly::notion::wnid == 103663910) // life support
      ;

  verbly::query<verbly::word> pictureQuery = database_->words(
    (verbly::notion::fullHypernyms %= whitelist)
    && !(verbly::notion::fullHypernyms %= blacklist)
    && (verbly::notion::partOfSpeech == verbly::part_of_speech::noun)
    && (verbly::notion::numOfImages >= 2));

  verbly::word pictured = pictureQuery.first();

  return pictured;
}

std::pair<Magick::Image, Magick::Image>
  difference::getImagesForNoun(verbly::word pictured) const
{
  std::cout << "Getting URLs..." << std::endl;

  std::vector<std::tuple<std::string, std::string>> images =
    imagenet_->getImagesForNotion(pictured.getNotion().getId(), rng_, 2);

  const std::string& imgdata1 = std::get<0>(images[0]);
  Magick::Blob image1(imgdata1.c_str(), imgdata1.length());
  Magick::Image pic1;
  pic1.read(image1);

  const std::string& imgdata2 = std::get<0>(images[1]);
  Magick::Blob image2(imgdata2.c_str(), imgdata2.length());
  Magick::Image pic2;
  pic2.read(image2);

  return {std::move(pic1), std::move(pic2)};
}

std::pair<verbly::word, verbly::word> difference::getOppositeIdentifiers() const
{
  verbly::part_of_speech pos;

  if (std::bernoulli_distribution(1.0/2.0)(rng_))
  {
    pos = verbly::part_of_speech::noun;
  } else {
    pos = verbly::part_of_speech::adjective;
  }

  verbly::word w1 = database_->words(
    (verbly::notion::partOfSpeech == pos)
    && (verbly::word::antonyms)).first();

  verbly::word w2 = database_->words(
    (verbly::notion::partOfSpeech == pos)
    && (verbly::word::antonyms %= w1)).first();

  return {std::move(w1), std::move(w2)};
}

Magick::Image difference::composeImage(
  Magick::Image image1,
  verbly::word word1,
  Magick::Image image2,
  verbly::word word2) const
{
  if (image1.columns() < 320)
  {
    image1.zoom(Magick::Geometry(320, image1.rows() * 320 / image1.columns(), 0, 0));
  }

  if (image2.columns() < 320)
  {
    image2.zoom(Magick::Geometry(320, image2.rows() * 320 / image2.columns(), 0, 0));
  }

  int width = std::min(image1.columns(), image2.columns());
  int height = std::min(image1.rows(), image2.rows());
  Magick::Geometry geo1(width, height, image1.columns()/2 - width/2, image1.rows()/2 - height/2);
  Magick::Geometry geo2(width, height, image2.columns()/2 - width/2, image2.rows()/2 - height/2);
  image1.crop(geo1);
  image2.crop(geo2);

  Magick::Image composite(Magick::Geometry(width*2, height, 0, 0), "white");
  composite.draw(Magick::DrawableCompositeImage(0, 0, image1));
  composite.draw(Magick::DrawableCompositeImage(width, 0, image2));
  composite.font(fontfile_);

  std::string ant1 = word1.getBaseForm().getText();
  std::string ant2 = word2.getBaseForm().getText();

  double fontsize = 72;
  for (;;)
  {
    composite.fontPointsize(fontsize);

    Magick::TypeMetric metric;
    composite.fontTypeMetrics(ant1, &metric);
    if (metric.textWidth() > 300)
    {
      fontsize -= 0.5;
      continue;
    }

    composite.fontTypeMetrics(ant2, &metric);
    if (metric.textWidth() > 300)
    {
      fontsize -= 0.5;
      continue;
    }

    break;
  }

  Magick::TypeMetric metric;
  composite.fontTypeMetrics(ant1, &metric);
  std::uniform_int_distribution<int> rowdist(20, (int)(composite.rows() - 19 - metric.textHeight()));
  int y = rowdist(rng_);
  y = composite.rows() - y;
  int x1 = (width - 40 - metric.textWidth())/2+20;
  composite.fontTypeMetrics(ant2, &metric);
  int x2 = (width - 40 - metric.textWidth())/2+20+width;
  composite.strokeColor("white");
  composite.strokeWidth(2);
  composite.antiAlias(false);
  composite.draw(Magick::DrawableText(x1, y, ant1));
  composite.draw(Magick::DrawableText(x2, y, ant2));

  composite.magick("png");

  return composite;
}

std::string difference::generatePostText(
  verbly::word word1,
  verbly::word word2) const
{
  verbly::token result {
    verbly::token::capitalize(verbly::token::casing::capitalize, word1),
    "vs.",
    verbly::token::capitalize(verbly::token::casing::capitalize, word2)
  };

  return result.compile();
}

void difference::sendPost(std::string text, Magick::Image image) const
{
  image.magick("png");
  image.write(tempfile_);

#ifdef ENABLE_BOT
  auto answer{connection_->post(mastodonpp::API::v1::media,
    {{"file", std::string("@file:") + tempfile_}, {"description", text}})};
  if (!answer)
  {
    if (answer.curl_error_code == 0)
    {
      std::cout << "HTTP status: " << answer.http_status << std::endl;
    }
    else
    {
      std::cout << "libcurl error " << std::to_string(answer.curl_error_code)
           << ": " << answer.error_message << std::endl;
    }
    return;
  }

  nlohmann::json response_json = nlohmann::json::parse(answer.body);
  answer = connection_->post(mastodonpp::API::v1::statuses,
    {{"status", ""}, {"media_ids",
      std::vector<std::string_view>{response_json["id"].get<std::string>()}}});

  if (!answer)
  {
    if (answer.curl_error_code == 0)
    {
      std::cout << "HTTP status: " << answer.http_status << std::endl;
    }
    else
    {
      std::cout << "libcurl error " << std::to_string(answer.curl_error_code)
           << ": " << answer.error_message << std::endl;
    }
  }
#endif
}
