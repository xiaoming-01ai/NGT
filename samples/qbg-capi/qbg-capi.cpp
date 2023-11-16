
#include	"NGT/Index.h"
#include	"NGT/NGTQ/Capi.h"
int
main(int argc, char **argv)
{
#if !defined(NGT_SHARED_MEMORY_ALLOCATOR)
  std::string indexPath  = "index";
  std::string objectFile = "sift-128-euclidean.tsv";
  std::string queryFile  = "query.tsv";

  CERR <<  " "  << "run the following commands to prepare data for this sample program." << std::endl;
  CERR <<  " "  << "  curl -L -O https://github.com/yahoojapan/NGT/raw/main/tests/datasets/ann-benchmarks/sift-128-euclidean.tsv" << std::endl;
  CERR <<  " "  << "  curl -L -O https://github.com/yahoojapan/NGT/raw/main/tests/datasets/ann-benchmarks/sift-128-euclidean_query.tsv" << std::endl;
  CERR <<  " "  << "  head -1 sift-128-euclidean_query.tsv > query.tsv" << std::endl;
  CERR <<  " "  << std::endl;
  CERR <<  " "  << "index path=" << indexPath << std::endl;
  CERR <<  " "  << "object file=" << objectFile << std::endl;
  CERR <<  " "  << "query file=" << queryFile << std::endl;
  CERR <<  " "  << std::endl;

  size_t dimension = 128;
  NGTError err = ngt_create_error_object();

  CERR <<  " "  << "create an empty index..." << std::endl;
  QBGConstructionParameters constructionParameters;
  qbg_initialize_construction_parameters(&constructionParameters);
  constructionParameters.dimension = dimension;
  constructionParameters.number_of_subvectors = 64;
  constructionParameters.number_of_blobs = 0;
  if (!qbg_create(indexPath.c_str(), &constructionParameters, err)) {
    CERR <<  " "  << "Cannot create" << std::endl;
    CERR <<  " "  << ngt_get_error_string(err) << std::endl;
    return 1;
  }

  CERR <<  " "  << "append objects..." << std::endl;
  auto index = qbg_open_index(indexPath.c_str(), false, err);
  if (index == 0) {
    CERR <<  " "  << "Cannot open" << std::endl;
    CERR <<  " "  << ngt_get_error_string(err) << std::endl;
    return 1;
  }

  try {
    std::ifstream is(objectFile);
    std::string	line;
    while (getline(is, line)) {
      std::vector<float> obj;
      std::stringstream	linestream(line);
      while (!linestream.eof()) {
	float value;
	linestream >> value;
	if (linestream.fail()) {
	  obj.clear();
	  break;
	}
	obj.push_back(value);
      }
      if (obj.empty()) {
	CERR <<  " "  << "An empty line or invalid value: " << line << std::endl;
	return 1;
      }
      if (qbg_append_object(index, obj.data(), dimension, err) == 0) {
	CERR <<  " "  << ngt_get_error_string(err) << std::endl;
	return 1;
      }
    }
  } catch (...) {
    CERR <<  " "  << "Error" << std::endl;
    return 1;
  }

  qbg_save_index(index, err);
  qbg_close_index(index);

  CERR <<  " "  << "building the index..." << std::endl;
  QBGBuildParameters buildParameters;
  qbg_initialize_build_parameters(&buildParameters);
  buildParameters.number_of_objects = 500;		
  auto status = qbg_build_index(indexPath.c_str(), &buildParameters, err);
  if (!status) {
    CERR <<  " "  << "Cannot build. " << ngt_get_error_string(err) << std::endl;
    return 1;
  }

  index = qbg_open_index(indexPath.c_str(), true, err);
  if (index == 0) {
    CERR <<  " "  << "Cannot open. " << ngt_get_error_string(err) << std::endl;
    return 1;
  }

  std::ifstream	is(queryFile);
  if (!is) {
    CERR <<  " "  << "Cannot open the specified file. " << queryFile << std::endl;
    return 1;
  }

  std::string line;
  float queryVector[dimension];
  if (getline(is, line)) {
    std::vector<double> queryObject;
    {
      std::vector<std::string> tokens;
      NGT::Common::tokenize(line, tokens, " \t");
      if (tokens.size() != dimension) {
	CERR <<  " "  << "dimension of the query is invalid. dimesion=" << tokens.size() << ":" << dimension << std::endl;
	return 1;
      }
      for (std::vector<std::string>::iterator ti = tokens.begin(); ti != tokens.end(); ++ti) {
	queryVector[distance(tokens.begin(), ti)] = NGT::Common::strtod(*ti);
      }
    }
    QBGObjectDistances result = ngt_create_empty_results(err);
    QBGQuery query;
    qbg_initialize_query(&query);
    query.query = &queryVector[0];
    CERR <<  " "  << "search the index for the specified query..." << std::endl;
    auto status = qbg_search_index(index, query, result, err);
    if (!status) {
      CERR <<  " "  << "Cannot search. " << ngt_get_error_string(err) << std::endl;
      return 1;
    }
    auto rsize = qbg_get_result_size(result, err);
    std::cout << "Rank\tID\tDistance" << std::endl;
    for (size_t i = 0; i < rsize; i++) {
      NGTObjectDistance object = qbg_get_result(result, i, err);
      std::cout << i + 1 << "\t" << object.id << "\t" << object.distance << std::endl;
    }

    qbg_destroy_results(result);
  }

  qbg_close_index(index);
  ngt_destroy_error_object(err);
#endif
  return 0;
}
