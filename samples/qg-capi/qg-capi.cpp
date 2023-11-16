
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

  NGTError err = ngt_create_error_object();
  NGTProperty prop = ngt_create_property(err);
  if (prop == NULL) {
    CERR <<  " "  << ngt_get_error_string(err) << std::endl;
    return 1;
  }
  size_t dimension = 128;
  ngt_set_property_dimension(prop, dimension, err);

  CERR <<  " "  << "create an empty index..." << std::endl;
  NGTIndex index = ngt_create_graph_and_tree(indexPath.c_str(), prop, err);
  if (index == NULL) {
    CERR <<  " "  << ngt_get_error_string(err) << std::endl;
    return 1;
  }

  CERR <<  " "  << "insert objects..." << std::endl;
  try {
    std::ifstream is(objectFile);
    std::string	line;
    while (getline(is, line)) {
      std::vector<double> obj;
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
      if (ngt_insert_index(index, obj.data(), dimension, err) == 0) {
	CERR <<  " "  << ngt_get_error_string(err) << std::endl;
	return 1;
      }
    }
  } catch (NGT::Exception &err) {
    CERR <<  " "  << "Error " << err.what() << std::endl;
    return 1;
  } catch (...) {
    CERR <<  " "  << "Error" << std::endl;
    return 1;
  }

  CERR <<  " "  << "build the index..." << std::endl;
  if (ngt_create_index(index, 100, err) == false) {
    CERR <<  " "  << "Error:" << ngt_get_error_string(err) << std::endl;
    return 1;
  }

  CERR <<  " "  << "save the index..." << std::endl;
  if (ngt_save_index(index, indexPath.c_str(), err) == false) {
    CERR <<  " "  << ngt_get_error_string(err) << std::endl;
    return 1;
  }

  CERR <<  " "  << "close the index..." << std::endl;
  ngt_close_index(index);

  NGTQGQuantizationParameters quantizationParameters;
  ngtqg_initialize_quantization_parameters(&quantizationParameters);

  CERR <<  " "  << "quantize the index..." << std::endl;
  if (ngtqg_quantize(indexPath.c_str(), quantizationParameters, err) == false) {
    CERR <<  " "  << ngt_get_error_string(err) << std::endl;
    return 1;
  }

  CERR <<  " "  << "open the quantized index..." << std::endl;
  index = ngtqg_open_index(indexPath.c_str(), err);
  if (index == NULL) {
    CERR <<  " "  << ngt_get_error_string(err) << std::endl;
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
    NGTObjectDistances result = ngt_create_empty_results(err);
    NGTQGQuery query;
    ngtqg_initialize_query(&query);
    query.query = queryVector;
    query.size = 10;
    query.result_expansion = 100;
    query.epsilon = 0.1;
    CERR <<  " "  << "search the index for the specified query..." << std::endl;
    ngtqg_search_index(index, query, result, err);

    auto rsize = ngt_get_result_size(result, err);
    std::cout << "Rank\tID\tDistance" << std::endl;
    for (size_t i = 0; i < rsize; i++) {
      NGTObjectDistance object = ngt_get_result(result, i, err);
      std::cout << i + 1 << "\t" << object.id << "\t" << object.distance << std::endl;
    }

    ngt_destroy_results(result);
  }

  CERR <<  " "  << "close the quantized index" << std::endl;
  ngtqg_close_index(index);
  ngt_destroy_error_object(err);
#endif
  return 0;
}
