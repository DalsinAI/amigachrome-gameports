/* Tests Enigma-facing behaviour, not only parser calls. MIT, Team 2026. */
#include "oxp_enigma.hh"
#include <cstdio>
#include <fstream>
using namespace oxpcompat;
static unsigned checks = 0, bad = 0;
#define CHECK(x)                                                               \
  do {                                                                         \
    ++checks;                                                                  \
    if (!(x)) {                                                                \
      ++bad;                                                                   \
      std::fprintf(stderr, "FAIL %d %s\n", __LINE__, #x);                      \
    }                                                                          \
  } while (0)
struct Errors : DOMErrorHandler {
  bool seen = false;
  bool handleError(const DOMError &) override {
    seen = true;
    return true;
  }
};
struct Resolver : DOMLSResourceResolver {
  std::string base;
  explicit Resolver(std::string b) : base(b) {}
  DOMLSInput *resolveResource(const XMLCh *, const XMLCh *, const XMLCh *,
                              const XMLCh *id, const XMLCh *) override {
    auto *i = new DOMLSInput;
    i->path = base + "/" + utf8(id);
    return i;
  }
};
struct Filter : DOMLSSerializerFilter {
  DOMNodeFilter::FilterAction acceptNode(const DOMNode *n) const override {
    return n->getNodeType() == DOMNode::ELEMENT_NODE &&
                   utf8(n->getNodeName()) == "property"
               ? DOMNodeFilter::FILTER_REJECT
               : DOMNodeFilter::FILTER_ACCEPT;
  }
  unsigned long getWhatToShow() const override {
    return DOMNodeFilter::SHOW_ALL;
  }
};
int main(int argc, char **argv) {
  std::setbuf(stdout, nullptr);
  std::setbuf(stderr, nullptr);
  if (argc != 2)
    return 2;
  Resolver resolver(argv[1]);
  Errors errors;
  DOMLSParser parser;
  parser.getDomConfig()->setParameter(XMLUni::fgDOMErrorHandler, &errors);
  parser.getDomConfig()->setParameter(XMLUni::fgDOMResourceResolver, &resolver);
  for (const char *f : {"enigmarc.xml", "state.xml", "score.xml", "ratings.xml",
                        "index.xml", "objects.xml"}) {
    std::string path = std::string(argv[1]) + "/" + f;
    std::printf("Reading %s\n", f);
    DOMDocument *d = parser.parseURI(path.c_str());
    CHECK(d);
    CHECK(!errors.seen);
    if (!d)
      continue;
    if (std::string(f) == "objects.xml") {
      auto *list = d->getElementsByTagName(u"attr");
      auto *el = dynamic_cast<DOMElement *>(list->item(0));
      CHECK(el);
      CHECK(utf8(el->getAttribute(u"min")) == "0");
      CHECK(!el->getAttributeNode(u"min")->getSpecified());
      DOMLSSerializer ser;
      XMLCh *s = ser.writeToString(d);
      std::string xml = utf8(s);
      XMLString::release(&s);
      oxp_doc *plain = oxp_dom_parse(xml.data(), xml.size(), 0, nullptr,
                                     nullptr, 0, nullptr, nullptr);
      CHECK(plain);
      auto *attrs = oxp_dom_child(oxp_dom_root(plain), nullptr, "attributes");
      CHECK(oxp_dom_attr(oxp_dom_child(attrs, nullptr, "attr"), nullptr,
                         "min") == nullptr);
      oxp_dom_free(plain);
      MemBufInputSource in(reinterpret_cast<const XMLByte *>(xml.data()),
                           xml.size(), "roundtrip", false);
      DOMDocument *again = parser.parse(&in);
      CHECK(again);
      CHECK(!dynamic_cast<DOMElement *>(
                 again->getElementsByTagName(u"attr")->item(0))
                 ->getAttributeNode(u"min")
                 ->getSpecified());
      again->release();
    }
    if (std::string(f) == "enigmarc.xml") {
      auto *props = dynamic_cast<DOMElement *>(
          d->getElementsByTagName(u"properties")->item(0));
      auto *list = props->getElementsByTagName(u"property");
      size_t before = list->getLength();
      auto *el = d->createElement(u"property");
      el->setAttribute(u"key", u"Unicode");
      el->setAttribute(u"value", u"Café 😀 & < >");
      props->appendChild(el);
      CHECK(list->getLength() == before + 1);
      CHECK(list->item(before) == el);
      DOMLSSerializer ser;
      XMLCh *s = ser.writeToString(d);
      std::string xml = utf8(s);
      XMLString::release(&s);
      MemBufInputSource in(reinterpret_cast<const XMLByte *>(xml.data()),
                           xml.size(), "roundtrip", false);
      auto *again = parser.parse(&in);
      CHECK(again);
      auto *value = dynamic_cast<DOMElement *>(
          again->getElementsByTagName(u"property")->item(before));
      CHECK(utf8(value->getAttribute(u"value")) == "Café 😀 & < >");
      again->release();
      Filter filter;
      ser.setFilter(&filter);
      s = ser.writeToString(d);
      xml = utf8(s);
      XMLString::release(&s);
      CHECK(xml.find("<property ") == std::string::npos);
      ser.setFilter(nullptr);
      props->removeChild(el);
      CHECK(list->getLength() == before);
      CHECK(ser.writeToURI(d, u"/this-directory-does-not-exist/test.xml") ==
            false);
    }
    std::printf("Releasing %s\n", f);
    d->release();
  }
  std::string wrong =
      "<state xmlns:xsi='http://www.w3.org/2001/XMLSchema-instance' "
      "xsi:noNamespaceSchemaLocation='state.xsd'><levels/></state>";
  MemBufInputSource in(reinterpret_cast<const XMLByte *>(wrong.data()),
                       wrong.size(), "invalid", false);
  CHECK(parser.parse(&in) == nullptr);
  CHECK(errors.seen);
  std::printf("Enigma/OXP adapter: %u checks, %u failures\n", checks, bad);
  return bad ? 1 : 0;
}
