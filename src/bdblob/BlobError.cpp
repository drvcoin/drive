/*
  Copyright (c) 2018 Drive Foundation

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

#include "BlobError.h"

namespace bdblob
{
  const char * BlobErrorToString(BlobError value)
  {
    switch (value)
    {
      case BlobError::SUCCESS:
        return "Success.";

      case BlobError::GENERIC_FAILURE:
        return "Some error occurred.";

      case BlobError::NOT_FOUND:
        return "File or folder not found.";

      case BlobError::IO_ERROR:
        return "I/O error.";

      case BlobError::ALREADY_EXIST:
        return "File or folder already exists.";

      case BlobError::NOT_SUPPORTED:
        return "Operation not supported.";

      case BlobError::FOLDER_NOT_EMPTY:
        return "Folder is not empty.";

      case BlobError::INVALID_NAME:
        return "Invalid file or directory name.";

      case BlobError::REJECT_DELETE_FOLDER:
        return "Cannot delete the target as it is a folder.";
    }  
  }
}