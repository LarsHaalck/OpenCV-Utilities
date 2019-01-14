#include "featureMatcher.h"

#include <iostream>

#include <opencv2/features2d.hpp>
#include <opencv2/xfeatures2d.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

namespace cvutils
{
    FeatureMatcher::FeatureMatcher(
        const std::filesystem::path& imgFolder,
        const std::filesystem::path& txtFile,
        const std::filesystem::path& ftFolder, int matcher,
        int window)
    : mHasTxtFile(!txtFile.string().empty())
    , mImgFolder(imgFolder)
    , mTxtFile(txtFile)
    , mFtFolder(ftFolder)
    , mMatcher(matcher)
    , mWindow(window)
{

    if (!std::filesystem::exists(mImgFolder)
        || !std::filesystem::is_directory(mImgFolder))
    {
        throw std::filesystem::filesystem_error("Img folder does not exist",
            mImgFolder, std::make_error_code(std::errc::no_such_file_or_directory));
    }

    if (mHasTxtFile && (!std::filesystem::exists(mTxtFile)
        || !std::filesystem::is_regular_file(mTxtFile)))
    {
        throw std::filesystem::filesystem_error("Txt file does not exist", mTxtFile,
            std::make_error_code(std::errc::no_such_file_or_directory));
    }

    if (!std::filesystem::exists(mFtFolder)
        || !std::filesystem::is_directory(mFtFolder))
    {
        throw std::filesystem::filesystem_error("Ft folder does not exist",
            mFtFolder, std::make_error_code(std::errc::no_such_file_or_directory));
    }
}

void FeatureMatcher::run()
{
    auto imgList = misc::getImgFiles(mImgFolder, mTxtFile);
    auto pairList = getPairList(imgList.size());
    auto feats = misc::getFtVecs(imgList, mFtFolder);
    auto descs = misc::getDescMats(imgList, mFtFolder);
    auto descMatcher = getMatcher();

    // NOTE: opencv doesnt support CV_32U
    cv::Mat pairMat = cv::Mat::zeros(pairList.size(), 2, CV_32S);
    size_t currMatId = 0;

    std::vector<std::vector<cv::DMatch>> matches;
    for (auto pair : pairList)
    {
        size_t i = pair.first;
        size_t j = pair.second;
        std::cout << "Matching: " << i << ", " << j << std::endl;
        std::vector<cv::DMatch> currMatches;
        descMatcher->match(descs[i], descs[j], currMatches);

        if (currMatches.size() >= 4)
        {
            pairMat.at<int>(currMatId, 0) = i;
            pairMat.at<int>(currMatId++, 1) = j;
            matches.push_back(currMatches);
        }
    }

    pairMat = cv::Mat(pairMat, cv::Range(0, currMatId));
    write(pairMat, matches, detail::MatchType::Putative);
}

//void FeatureMatcher::test(const std::vector<std::string>& imgList)
//{
//    auto feats = misc::getFtVecs(imgList, mFtFolder);
//    auto matchPair = misc::getMatches(mFtFolder, detail::MatchType::Putative);
//    const auto& pairMat = matchPair.first;
//    const auto& matches = matchPair.second;
//    for(int k = 0; k < pairMat.rows; k++)
//    {
//        size_t i  = pairMat.at<int>(k, 0);
//        size_t j = pairMat.at<int>(k, 1);
//        cv::Mat imgI = cv::imread(imgList[i]);
//        cv::Mat imgJ = cv::imread(imgList[j]);
//
//        cv::Mat res;
//        cv::drawMatches(imgI, feats[i], imgJ, feats[j], matches[k], res);
//        cv::imshow("bla", res);
//        cv::waitKey(0);
//    }
//}

std::vector<std::pair<size_t, size_t>> FeatureMatcher::getPairList(size_t size)
{
    if (mWindow != 0)
        return getWindowPairList(size);
    else
        return getExhaustivePairList(size);
}

std::vector<std::pair<size_t, size_t>> FeatureMatcher::getWindowPairList(size_t size)
{
    std::vector<std::pair<size_t, size_t>> pairList;
    for (size_t i = 0; i < size; i++)
    {
        for (size_t j = i + 1; (j < i + mWindow) && (j < size); j++)
        {
            pairList.push_back(std::make_pair(i, j));
        }
    }
    return pairList;
}

std::vector<std::pair<size_t, size_t>> FeatureMatcher::getExhaustivePairList(size_t size)
{
    std::vector<std::pair<size_t, size_t>> pairList;
    for (size_t i = 0; i < size; i++)
    {
        for (size_t j = i + 1; j < size; j++)
        {
            pairList.push_back(std::make_pair(i, j));
        }
    }
    return pairList;
}

cv::Ptr<cv::DescriptorMatcher> FeatureMatcher::getMatcher()
{
    if (mMatcher)
    {
        return cv::DescriptorMatcher::create(
            cv::DescriptorMatcher::MatcherType::FLANNBASED);
    }
    else
    {
        return cv::DescriptorMatcher::create(
            cv::DescriptorMatcher::MatcherType::BRUTEFORCE);

    }
}

void FeatureMatcher::write(const cv::Mat& pairMat,
    const std::vector<std::vector<cv::DMatch>>& matches, detail::MatchType type)
{
    std::string ending = (type == detail::MatchType::Putative)
        ? "-putative.yml"
        : "-geometric.yml";

    std::filesystem::path base("matches");
    base = mFtFolder / base;
    cv::FileStorage matchFile(base.string() + ending,
        cv::FileStorage::WRITE);

    if (!matchFile.isOpened())
    {
        std::cout << "Could not open matches file for writing" << std::endl;
        return;
    }
    cv::write(matchFile, "pairMat", pairMat);
    for (size_t i = 0; i < matches.size(); i++)
    {
        cv::write(matchFile, std::string("matches_") + std::to_string(i), matches[i]);
    }

}

}