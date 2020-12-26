//#include "testresultreporter.h"
//#include "test.h"
//#include "testresult.h"
//#include <boost/algorithm/string.hpp>

//TestResultReporter::TestResultReporter(int numberOfTests, const fs::path& reportFile)
//    : numberOfTests_(numberOfTests)
//{
//}

//void TestResultReporter::reportResult(const Test &test, const TestResult &result)
//{
//    auto report = TestReport{};
//    report.name = test.name();
//    report.index = getTestIndex(test.suite());
//    if (result.type() == TestResultType::Success)
//        testSuiteReportMap_[test.suite()].successfulTestReports.push_back(report);

//    if (result.type() != TestResultType::Success){
//        auto detailedReport = DetailedTestReport{};
//        detailedReport.index = report.index;
//        detailedReport.name = report.name;
//        detailedReport.description = test.description();
//        detailedReport.result = result.type();
//        if (result.type() == TestResultType::RuntimeError)
//            detailedReport.errorInfo = result.errorInfo();
//        else if (result.type() == TestResultType::Failure)
//            detailedReport.errorInfo = boost::join(result.failedActionsMessages(), "\n");
//        testSuiteReportMap_[test.suite()].unsuccessfulTestReports.push_back(detailedReport);
//    }
//}

//int TestResultReporter::getTestIndex(const std::string suite)
//{
//    auto it = testSuiteReportMap_.find(suite);
//    if (it == testSuiteReportMap_.end())
//        return 0;
//    else{
//        auto& report = it->second;
//        return static_cast<int>(report.successfulTestReports.size() +
//                                report.unsuccessfulTestReports.size());
//    }
//}
