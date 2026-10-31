# Existing OpenXML implementation — checked 31 October 2026

Repository: https://github.com/DalsinAI/openamigaxml
Checked main: 2db519bed58662d3462d33fdc373de130e4ac635.

The existing oxp/ directory contains the Team's own XML parser, tree and writer.
Do not implement a second parser. The earlier proposal to add parsing, namespace,
Unicode and serialization support from scratch was based on an incomplete inventory.

Fresh checks on DaleTop: build-host.sh passed its API tests and all 70 cases;
build-amiga.sh built liboxp.a and the two native test programs with Open3.
This is a new cross-build check, not a new guest runtime check. The repository
also records prior AmigaOS 3.2.3 target tests and W3C conformance results.

Existing capabilities: incremental XML 1.0 parsing, namespaces, UTF-8 and UTF-16,
entity limits, errors with locations, DOM lookup/traversal/text and XML writing.

Enigma-specific work remaining: adapter for its existing Xerces-facing calls;
owned tree construction/mutation/import API (not exposed by current oxp_dom.h);
XSD validation (explicitly excluded by the current OXP README); file persistence
and Enigma level/settings/score integration and runtime tests. Reuse the current
parser and extend only verified gaps.
