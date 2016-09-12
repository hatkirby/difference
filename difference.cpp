#include <verbly.h>
#include <twitter.h>
#include <curl_easy.h>
#include <curl_header.h>
#include <list>
#include <set>
#include <vector>
#include <algorithm>
#include <Magick++.h>
#include <iostream>
#include <yaml-cpp/yaml.h>
#include <chrono>
#include <thread>
#include <random>

std::string capitalize(std::string input)
{
  std::string result;
  bool capnext = true;
  
  for (auto ch : input)
  {
    if (capnext)
    {
      result += toupper(ch);
      capnext = false;
    } else {
      result += ch;
    }
    
    if ((ch == ' ') || (ch == '-'))
    {
      capnext = true;
    }
  }
  
  return result;
}

bool downloadImage(std::string url, curl::curl_header headers, Magick::Blob& img, Magick::Image& pic)
{
  // willyfogg.com is a thumbnail generator known to return 200 even if the target image no longer exists
  if (url.find("willyfogg.com/thumb.php") != std::string::npos)
  {
    return false;
  }
  
  std::ostringstream imgbuf;
  curl::curl_ios<std::ostringstream> imgios(imgbuf);
  curl::curl_easy imghandle(imgios);
  
  imghandle.add<CURLOPT_HTTPHEADER>(headers.get());
  imghandle.add<CURLOPT_URL>(url.c_str());
  imghandle.add<CURLOPT_CONNECTTIMEOUT>(30);
  
  try {
    imghandle.perform();
  } catch (curl::curl_easy_exception error) {
    error.print_traceback();
  
    return false;
  }

  if (imghandle.get_info<CURLINFO_RESPONSE_CODE>().get() != 200)
  {
    return false;
  }
  
  std::string content_type = imghandle.get_info<CURLINFO_CONTENT_TYPE>().get();
  if (content_type.substr(0, 6) != "image/")
  {
    return false;
  }
  
  std::string imgstr = imgbuf.str();
  img = Magick::Blob(imgstr.c_str(), imgstr.length());
  pic.read(img);
  if (pic.rows() == 0)
  {
    return false;
  }
  
  std::cout << url << std::endl;
  
  return true;
}

int main(int argc, char** argv)
{
  Magick::InitializeMagick(nullptr);
  
  int delay = 60 * 60;
  
  YAML::Node config = YAML::LoadFile("config.yml");
  twitter::auth auth;
  auth.setConsumerKey(config["consumer_key"].as<std::string>());
  auth.setConsumerSecret(config["consumer_secret"].as<std::string>());
  auth.setAccessKey(config["access_key"].as<std::string>());
  auth.setAccessSecret(config["access_secret"].as<std::string>());
  
  twitter::client client(auth);
  
  std::random_device random_device;
  std::mt19937 random_engine{random_device()};
  
  verbly::data database("data.sqlite3");
  
  auto whitelist = database.nouns();
  whitelist.with_wnid(111530512); // Crops (plants)
  whitelist.with_wnid(111536087); // Ornamental (plants)
  whitelist.with_wnid(111536230); // Pot plants
  whitelist.with_wnid(113083023); // Houseplants
  whitelist.with_wnid(113083306); // Garden plants
  whitelist.with_wnid(113083586); // Vascular plants (includes flowers)
  whitelist.with_wnid(109287968); // Geological formations
  whitelist.with_wnid(109208496); // Asterisms (collections of stars)
  whitelist.with_wnid(109239740); // Celestial bodies
  whitelist.with_wnid(109277686); // Exterrestrial objects (comets and meteroids)
  whitelist.with_wnid(109403211); // Radiators (supposedly natural radiators but actually these are just pictures of radiators)
  whitelist.with_wnid(109416076); // Rocks
  whitelist.with_wnid(105442131); // Chromosomes
  whitelist.with_wnid(100324978); // Tightrope walking
  whitelist.with_wnid(100326094); // Rock climbing
  whitelist.with_wnid(100433458); // Contact sports
  whitelist.with_wnid(100433802); // Gymnastics
  whitelist.with_wnid(100439826); // Track and field
  whitelist.with_wnid(100440747); // Skiing
  whitelist.with_wnid(100441824); // Water sport
  whitelist.with_wnid(100445351); // Rowing
  whitelist.with_wnid(100446980); // Archery
  // TODO: add more sports
  whitelist.with_wnid(100021939); // Artifacts
  whitelist.with_wnid(101471682); // Vertebrates
  
  auto whitedata = whitelist.run();
  std::set<int> wnids;
  verbly::filter<verbly::noun> whitefilter;
  whitefilter.set_orlogic(true);
  for (auto wd : whitedata)
  {
    if (wnids.count(wd.wnid()) == 0)
    {
      whitefilter << wd;
      wnids.insert(wd.wnid());
    }
  }
  
  // Accept string from Google Chrome
  std::string accept = "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8";
  curl::curl_header headers;
  headers.add(accept);
  
  std::cout << "Started!" << std::endl;
  for (;;)
  {
    try
    {
      std::cout << "Generating noun..." << std::endl;
      verbly::noun pictured = database.nouns().full_hyponym_of(whitefilter).at_least_n_images(2).random().limit(1).run().front();
      std::cout << "Noun: " << pictured.singular_form() << std::endl;
      std::cout << "Getting URLs..." << std::endl;
      std::ostringstream lstbuf;
      curl::curl_ios<std::ostringstream> lstios(lstbuf);
      curl::curl_easy lsthandle(lstios);
      std::string lsturl = pictured.imagenet_url();
      lsthandle.add<CURLOPT_URL>(lsturl.c_str());
      lsthandle.perform();
    
      if (lsthandle.get_info<CURLINFO_RESPONSE_CODE>().get() != 200)
      {
        std::cout << "Could not get URLs" << std::endl;
        continue;
      }
    
      std::cout << "Got URLs." << std::endl;
      std::string lstdata = lstbuf.str();
      auto lstlist = verbly::split<std::list<std::string>>(lstdata, "\r\n");
      std::set<std::string> lstset;
      for (auto url : lstlist)
      {
        lstset.insert(url);
      }
    
      if (lstset.size() < 2)
      {
        continue;
      }
    
      std::vector<std::string> lstvec;
      for (auto url : lstset)
      {
        lstvec.push_back(url);
      }
    
      std::shuffle(std::begin(lstvec), std::end(lstvec), random_engine);
    
      Magick::Blob img1;
      Magick::Image pic1;
      bool success = false;
      int curind = 0;
      for (; curind < lstvec.size(); curind++)
      {
        if (downloadImage(lstvec[curind], headers, img1, pic1))
        {
          success = true;
          break;
        }
      }
    
      if (!success)
      {
        continue;
      }
    
      success = false;
      Magick::Blob img2;
      Magick::Image pic2;
      for (curind++; curind < lstvec.size(); curind++)
      {
        if (downloadImage(lstvec[curind], headers, img2, pic2))
        {
          success = true;
          break;
        }
      }
    
      if (!success)
      {
        continue;
      }
    
      std::string ant1, ant2;
      std::uniform_int_distribution<int> coinflip(0, 1);
      if (coinflip(random_engine)==0)
      {
        verbly::noun n1 = database.nouns().has_antonyms().random().limit(1).run().front();
        verbly::noun n2 = n1.antonyms().random().limit(1).run().front();
        ant1 = n1.singular_form();
        ant2 = n2.singular_form();
      } else {
        verbly::adjective a1 = database.adjectives().has_antonyms().random().limit(1).run().front();
        verbly::adjective a2 = a1.antonyms().random().limit(1).run().front();
        ant1 = a1.base_form();
        ant2 = a2.base_form();
      }
    
      if (pic1.columns() < 320)
      {
        pic1.zoom(Magick::Geometry(320, pic1.rows() * 320 / pic1.columns(), 0, 0));
      }
    
      if (pic2.columns() < 320)
      {
        pic2.zoom(Magick::Geometry(320, pic2.rows() * 320 / pic2.columns(), 0, 0));
      }
    
      int width = std::min(pic1.columns(), pic2.columns());
      int height = std::min(pic1.rows(), pic2.rows());
      Magick::Geometry geo1(width, height, pic1.columns()/2 - width/2, pic1.rows()/2 - height/2);
      Magick::Geometry geo2(width, height, pic2.columns()/2 - width/2, pic2.rows()/2 - height/2);
      pic1.crop(geo1);
      pic2.crop(geo2);
    
      Magick::Image composite(Magick::Geometry(width*2, height, 0, 0), "white");
      composite.draw(Magick::DrawableCompositeImage(0, 0, pic1));
      composite.draw(Magick::DrawableCompositeImage(width, 0, pic2));
      composite.font("@coolvetica.ttf");
    
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
      int y = rowdist(random_engine);
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
    
      Magick::Blob outputimg;
      composite.write(&outputimg);
    
      std::cout << "Generated image!" << std::endl << "Tweeting..." << std::endl;
    
      std::stringstream msg;
      msg << capitalize(ant1);
      msg << " vs. ";
      msg << capitalize(ant2);

      long media_id = client.uploadMedia("image/png", (const char*) outputimg.data(), outputimg.length());
      client.updateStatus(msg.str(), {media_id});

      std::cout << "Done!" << std::endl << "Waiting..." << std::endl << std::endl;
    } catch (const curl::curl_easy_exception& error)
    {
      error.print_traceback();
    } catch (const twitter::twitter_error& e)
    {
      std::cout << "Twitter error: " << e.what() << std::endl;
    }
    
    std::this_thread::sleep_for(std::chrono::seconds(delay));
  }
  
  return 0;
}
