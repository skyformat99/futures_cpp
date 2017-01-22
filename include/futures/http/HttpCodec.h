#pragma once

#include <unordered_map>
#include <futures/io/IoFuture.h>

namespace futures {
namespace http {

struct Parser;

struct Request {
    unsigned int err;
    unsigned int method;
    uint64_t content_length;
    std::string url;
    std::unordered_map<std::string, std::string> headers;

    void reset() {
        content_length = 0;
        headers.clear();
        url.clear();
    }

    friend std::ostream& operator<< (std::ostream& stream, const Request& matrix);
};

std::ostream& operator<< (std::ostream& stream, const Request& o);

struct Response {
    unsigned int http_errno;
    std::unordered_map<std::string, std::string> headers;
    folly::IOBufQueue body;

    Response()
      : body(folly::IOBufQueue::cacheChainLength())
    {}
};

class HttpV1Codec: public io::Codec<HttpV1Codec, Request, int64_t> {
public:
    using In = Request;
    using Out = Response;

    HttpV1Codec();
    ~HttpV1Codec();

    Try<Optional<In>> decode(std::unique_ptr<folly::IOBuf> &buf);

    Try<void> encode(Out& out,
            folly::IOBufQueue &buf);

    HttpV1Codec(HttpV1Codec&&);
    HttpV1Codec& operator=(HttpV1Codec&&);
private:
    std::unique_ptr<Parser> impl_;
};

#if 0
class HttpV1Handler : public InboundHandler<folly::IOBufQueue&, Request> {
public:
  typedef typename InboundHandler<folly::IOBufQueue&, Request>::Context Context;
  void read(Context* ctx, folly::IOBufQueue &msg) override;

  HttpV1Handler();
  ~HttpV1Handler();
private:
    std::unique_ptr<Parser> impl_;
};
#endif

}
}
