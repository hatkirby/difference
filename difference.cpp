#include <verbly.h>
#include <twitter.h>
#include <curl_easy.h>
#include <list>
#include <set>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <Magick++.h>
#include <iostream>
#include <yaml-cpp/yaml.h>
#include <unistd.h>

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

int main(int argc, char** argv)
{
  srand(time(NULL));
  Magick::InitializeMagick(nullptr);
  
  int delay = 60 * 60;
  
  YAML::Node config = YAML::LoadFile("config.yml");
  twitter::auth auth;
  auth.setConsumerKey(config["consumer_key"].as<std::string>());
  auth.setConsumerSecret(config["consumer_secret"].as<std::string>());
  auth.setAccessKey(config["access_key"].as<std::string>());
  auth.setAccessSecret(config["access_secret"].as<std::string>());
  
  twitter::client client(auth);
  
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
  
  std::cout << "Started!" << std::endl;
  for (;;)
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
    try {
      lsthandle.perform();
    } catch (curl::curl_easy_exception error) {
      error.print_traceback();
      sleep(delay);
      
      continue;
    }
    
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
    
    std::random_shuffle(std::begin(lstvec), std::end(lstvec));
    
    Magick::Blob img1;
    Magick::Image pic1;
    bool success = false;
    int curind = 0;
    for (; curind < lstvec.size(); curind++)
    {
      std::ostringstream img1buf;
      curl::curl_ios<std::ostringstream> img1ios(img1buf);
      curl::curl_easy img1handle(img1ios);
      std::string img1url = lstvec[curind];
      img1handle.add<CURLOPT_URL>(img1url.c_str());
      img1handle.add<CURLOPT_CONNECTTIMEOUT>(30);
      
      try {
        img1handle.perform();
      } catch (curl::curl_easy_exception error) {
        error.print_traceback();
      
        continue;
      }
    
      if (img1handle.get_info<CURLINFO_RESPONSE_CODE>().get() != 200)
      {
        continue;
      }
      
      if (std::string(img1handle.get_info<CURLINFO_CONTENT_TYPE>().get()).substr(0, 6) != "image/")
      {
        continue;
      }
      
      std::string img1str = img1buf.str();
      img1 = Magick::Blob(img1str.c_str(), img1str.length());
      pic1.read(img1);
      if (pic1.rows() == 0)
      {
        continue;
      }
      
      std::cout << img1url << std::endl;
      success = true;
      
      break;
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
      std::ostringstream img2buf;
      curl::curl_ios<std::ostringstream> img2ios(img2buf);
      curl::curl_easy img2handle(img2ios);
      std::string img2url = lstvec[curind];
      img2handle.add<CURLOPT_URL>(img2url.c_str());
      img2handle.add<CURLOPT_CONNECTTIMEOUT>(30);
      
      try {
        img2handle.perform();
      } catch (curl::curl_easy_exception error) {
        error.print_traceback();
      
        continue;
      }
    
      if (img2handle.get_info<CURLINFO_RESPONSE_CODE>().get() != 200)
      {
        continue;
      }
      
      if (std::string(img2handle.get_info<CURLINFO_CONTENT_TYPE>().get()).substr(0, 6) != "image/")
      {
        continue;
      }

      std::string img2str = img2buf.str();
      img2 = Magick::Blob(img2str.c_str(), img2str.length());
      pic2.read(img2);
      if (pic2.rows() == 0)
      {
        continue;
      }
      
      std::cout << img2url << std::endl;
      success = true;
      
      break;
    }
    
    if (!success)
    {
      continue;
    }
    
    std::string ant1, ant2;
    if (rand()%2==0)
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
    int y = rand() % ((int)(composite.rows() - 40 - metric.textHeight())) + 20;
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
    
    long media_id;
    twitter::response resp = client.uploadMedia("image/png", (const char*) outputimg.data(), outputimg.length(), media_id);
    if (resp != twitter::response::ok)
    {
      std::cout << "Twitter error while uploading image: " << resp << std::endl;
      sleep(delay);
      
      continue;
    }
    
    twitter::tweet tw;
    resp = client.updateStatus(msg.str(), tw, {media_id});
    if (resp != twitter::response::ok)
    {
      std::cout << "Twitter error while tweeting: " << resp << std::endl;
      sleep(delay);
      
      continue;
    }
    
    std::cout << "Done!" << std::endl << "Waiting..." << std::endl << std::endl;
    
    sleep(delay);
  }
  
  return 0;
}
