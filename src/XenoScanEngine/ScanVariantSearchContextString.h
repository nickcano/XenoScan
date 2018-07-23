/*	// TODO 1: clean up, maybe work into comparator system? essentially, move all
	//         logic into the ScanVariantTypeTraits implementations
	// TODO 2: we should pre-initialize the array of character indices before
	//         the entire search rather than doing it for every chunk
	// TODO 3: get cmake working with /std:c++17
	if (traits->isStringType() && compType == Scanner::SCAN_COMPARE_EQUALS)
	{
		if (this->type == SCAN_VARIANT_ASCII_STRING)
		{
			auto schunk = std::string(&chunk[0], &chunk[chunkSize]);
			auto searcher = std::boyer_moore_horspool_searcher<std::string::const_iterator>(this->valueAsciiString.cbegin(), this->valueAsciiString.cend());
			auto res = schunk.begin();

			while (true)
			{
				res = std::search(res, schunk.end(), searcher);
				if (res == schunk.end())
					break;
				locations.push_back(res - schunk.begin());
				res++;
			}
			return;
		}
	}*/