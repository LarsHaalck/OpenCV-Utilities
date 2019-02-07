#ifndef CVUTILS_FEATURE_FETCHER_H
#define CVUTILS_FEATURE_FETCHER_H

#include <filesystem>
#include <vector>

#include <opencv2/core.hpp>

#include "fetch/abstractFetcher.h"
#include "fetch/imageFetcher.h"
#include "io/config.h"

namespace cvutils
{
namespace detail
{
class FeatureFetcher : public AbstractFetcher<size_t, std::vector<cv::KeyPoint>>
{
private:
    ImageFetcher mImgFetcher;
    std::filesystem::path mFtDir;
public:
    FeatureFetcher(const std::filesystem::path& imgDir,
        const std::filesystem::path& txtFile, const std::filesystem::path& ftDir)
    : mImgFetcher(imgDir, txtFile)
    , mFtDir(ftDir)
    {
        if (!std::filesystem::exists(mFtDir)
            || !std::filesystem::is_directory(mFtDir))
        {
            throw std::filesystem::filesystem_error("Feature folder does not exist",
                mFtDir, std::make_error_code(std::errc::no_such_file_or_directory));
        }
    }

    size_t size() const override { return mImgFetcher.size(); }

    std::vector<cv::KeyPoint> get(const size_t& idx) const override
    {
        std::filesystem::path imgStem(mImgFetcher.getImagePath(idx));
        imgStem = mFtDir / imgStem.stem();

        cv::FileStorage fsFt(imgStem.string() + detail::ftEnding, cv::FileStorage::READ);
        std::vector<cv::KeyPoint> currKeyPts;
        cv::read(fsFt[detail::ftKey], currKeyPts);

        return currKeyPts;
    }

};
} // namespace detail
} // namespace cvutils

#endif // CVUTILS_FEATURE_FETCHER_H