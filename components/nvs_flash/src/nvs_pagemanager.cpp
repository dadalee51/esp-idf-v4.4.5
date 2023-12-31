// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include "nvs_pagemanager.hpp"

namespace nvs
{
esp_err_t PageManager::load(uint32_t baseSector, uint32_t sectorCount)
{
    mBaseSector = baseSector;
    mPageCount = sectorCount;
    mPageList.clear();
    mFreePageList.clear();
    mPages.reset(new Page[sectorCount]);

    for (uint32_t i = 0; i < sectorCount; ++i) {
        auto err = mPages[i].load(baseSector + i);
        if (err != ESP_OK) {
            return err;
        }
        uint32_t seqNumber;
        if (mPages[i].getSeqNumber(seqNumber) != ESP_OK) {
            mFreePageList.push_back(&mPages[i]);
        } else {
            auto pos = std::find_if(std::begin(mPageList), std::end(mPageList), [=](const Page& page) -> bool {
                uint32_t otherSeqNumber;
                return page.getSeqNumber(otherSeqNumber) == ESP_OK && otherSeqNumber > seqNumber;
            });
            if (pos == mPageList.end()) {
                mPageList.push_back(&mPages[i]);
            } else {
                mPageList.insert(pos, &mPages[i]);
            }
        }
    }

    if (mPageList.empty()) {
        mSeqNumber = 0;
        return activatePage();
    }
    else {
        uint32_t lastSeqNo;
        assert(mPageList.back().getSeqNumber(lastSeqNo) == ESP_OK);
        mSeqNumber = lastSeqNo + 1;
    }

    return ESP_OK;
}

esp_err_t PageManager::requestNewPage()
{
    if (mFreePageList.empty()) {
        return ESP_ERR_NVS_INVALID_STATE;
    }

    // do we have at least two free pages? in that case no erasing is required
    if (mFreePageList.size() >= 2) {
        return activatePage();
    }

    // find the page with the higest number of erased items
    TPageListIterator maxErasedItemsPageIt;
    size_t maxErasedItems = 0;
    for (auto it = begin(); it != end(); ++it) {
        auto erased = it->getErasedEntryCount();
        if (erased > maxErasedItems) {
            maxErasedItemsPageIt = it;
            maxErasedItems = erased;
        }
    }

    if (maxErasedItems == 0) {
        return ESP_ERR_NVS_NOT_ENOUGH_SPACE;
    }

    esp_err_t err = activatePage();
    if (err != ESP_OK) {
        return err;
    }
    
    Page* newPage = &mPageList.back();

    Page* erasedPage = maxErasedItemsPageIt;
    err = erasedPage->markFreeing();
    if (err != ESP_OK) {
        return err;
    }
    while (true) {
        err = erasedPage->moveItem(*newPage);
        if (err == ESP_ERR_NVS_NOT_FOUND) {
            break;
        } else if (err != ESP_OK) {
            return err;
        }
    }

    err = erasedPage->erase();
    if (err != ESP_OK) {
        return err;
    }

    mPageList.erase(maxErasedItemsPageIt);
    mFreePageList.push_back(erasedPage);

    return ESP_OK;
}

esp_err_t PageManager::activatePage()
{
    if (mFreePageList.empty()) {
        return ESP_ERR_NVS_NOT_ENOUGH_SPACE;
    }
    Page* p = &mFreePageList.front();
    if (p->state() == Page::PageState::CORRUPT) {
        auto err = p->erase();
        if (err != ESP_OK) {
            return err;
        }
    }
    mFreePageList.pop_front();
    mPageList.push_back(p);
    p->setSeqNumber(mSeqNumber);
    ++mSeqNumber;
    return ESP_OK;
}
    
} // namespace nvs
